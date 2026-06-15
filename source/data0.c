/* data0.c -- read-only mount of the bundled asset pack
 *
 * Some assets (notably the nazo/ puzzle graphics) ship only inside assets/data0,
 * a ZIP whose game files are STORED under "files/assetpacks/.../assets/<rel>".
 * We index it once and serve any asset that has no loose file, keyed by <rel>
 * (the part after "/assets/"); STORED entries are a plain seek + read.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "data0.h"
#include "util.h"

typedef struct {
  char *rel;        // path after "/assets/"
  uint32_t size;    // stored == uncompressed size
  uint32_t lho;     // local file header offset
  int64_t data_off; // resolved file-data offset, -1 until first use
} Entry;

static FILE *g_f;
static Mutex g_lock;
static Entry *g_ent;
static int g_count;

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int data0_mount(const char *path) {
  g_f = fopen(path, "rb");
  if (!g_f)
    return -1;
  mutexInit(&g_lock);

  if (fseek(g_f, 0, SEEK_END) != 0)
    goto fail;
  const long fsize = ftell(g_f);
  if (fsize < 22)
    goto fail;

  // find the End Of Central Directory record (sig "PK\5\6") near EOF
  long scan = fsize < 65557 ? fsize : 65557;
  uint8_t *tail = malloc(scan);
  if (!tail)
    goto fail;
  fseek(g_f, fsize - scan, SEEK_SET);
  if (fread(tail, 1, scan, g_f) != (size_t)scan) { free(tail); goto fail; }
  long e = -1;
  for (long i = scan - 22; i >= 0; i--) {
    if (tail[i] == 0x50 && tail[i+1] == 0x4b && tail[i+2] == 0x05 && tail[i+3] == 0x06) {
      e = i;
      break;
    }
  }
  if (e < 0) { free(tail); goto fail; }
  const uint32_t total   = rd16(tail + e + 10);
  const uint32_t cd_size = rd32(tail + e + 12);
  const uint32_t cd_off  = rd32(tail + e + 16);
  free(tail);
  if (total == 0 || cd_size == 0)
    goto fail;

  // read the whole central directory
  uint8_t *cd = malloc(cd_size);
  if (!cd)
    goto fail;
  fseek(g_f, cd_off, SEEK_SET);
  if (fread(cd, 1, cd_size, g_f) != cd_size) { free(cd); goto fail; }

  g_ent = malloc(sizeof(Entry) * total);
  if (!g_ent) { free(cd); goto fail; }
  g_count = 0;

  uint32_t p = 0;
  for (uint32_t i = 0; i < total && p + 46 <= cd_size; i++) {
    if (!(cd[p] == 0x50 && cd[p+1] == 0x4b && cd[p+2] == 0x01 && cd[p+3] == 0x02))
      break;
    const uint32_t usize = rd32(cd + p + 24);
    const uint16_t fnl   = rd16(cd + p + 28);
    const uint16_t exl   = rd16(cd + p + 30);
    const uint16_t cml   = rd16(cd + p + 32);
    const uint32_t lho   = rd32(cd + p + 42);
    const char *name = (const char *)(cd + p + 46);

    // game files live under ".../assets/<relpath>"; index them by <relpath>
    const char *hit = NULL;
    for (int k = 0; k + 8 <= (int)fnl; k++) {
      if (memcmp(name + k, "/assets/", 8) == 0) { hit = name + k + 8; break; }
    }
    if (hit && (int)(hit - name) < (int)fnl) {
      const int rlen = (int)fnl - (int)(hit - name);
      char *rel = malloc(rlen + 1);
      if (rel) {
        memcpy(rel, hit, rlen);
        rel[rlen] = '\0';
        g_ent[g_count].rel = rel;
        g_ent[g_count].size = usize;
        g_ent[g_count].lho = lho;
        g_ent[g_count].data_off = -1;
        g_count++;
      }
    }
    p += 46u + fnl + exl + cml;
  }
  free(cd);
  debugPrintf("data0: mounted %d entries from %s\n", g_count, path);
  return g_count;

fail:
  if (g_f) { fclose(g_f); g_f = NULL; }
  free(g_ent);
  g_ent = NULL;
  g_count = 0;
  return -1;
}

static Entry *find_entry(const char *rel) {
  if (!g_f || !g_ent || !rel)
    return NULL;
  for (int i = 0; i < g_count; i++)
    if (g_ent[i].rel[0] == rel[0] && strcmp(g_ent[i].rel, rel) == 0)
      return &g_ent[i];
  return NULL;
}

// resolve an entry's file-data offset by reading its local header once
static int64_t resolve(Entry *en) {
  if (en->data_off >= 0)
    return en->data_off;
  uint8_t lf[30];
  mutexLock(&g_lock);
  int ok = (fseek(g_f, en->lho, SEEK_SET) == 0) && (fread(lf, 1, 30, g_f) == 30);
  mutexUnlock(&g_lock);
  if (!ok || !(lf[0]==0x50 && lf[1]==0x4b && lf[2]==0x03 && lf[3]==0x04))
    return -1;
  en->data_off = (int64_t)en->lho + 30 + rd16(lf + 26) + rd16(lf + 28);
  return en->data_off;
}

int data0_locate(const char *rel, int64_t *abs_off, int64_t *size) {
  Entry *en = find_entry(rel);
  if (!en)
    return 0;
  const int64_t off = resolve(en);
  if (off < 0)
    return 0;
  if (abs_off) *abs_off = off;
  if (size) *size = en->size;
  return 1;
}

int data0_pread(void *buf, int64_t abs_off, int len) {
  if (!g_f || len < 0)
    return -1;
  mutexLock(&g_lock);
  size_t got = 0;
  if (fseek(g_f, (long)abs_off, SEEK_SET) == 0)
    got = fread(buf, 1, (size_t)len, g_f);
  else
    got = (size_t)-1;
  mutexUnlock(&g_lock);
  return (got == (size_t)-1) ? -1 : (int)got;
}

int data0_dup_fd(void) {
  if (!g_f)
    return -1;
  return dup(fileno(g_f));
}
