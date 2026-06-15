/* data0.h -- read-only mount of the bundled asset pack (assets/data0)
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef __DATA0_H__
#define __DATA0_H__

#include <stdint.h>

// index the pack; returns entry count, or -1 if absent/unreadable
int data0_mount(const char *path);

// look up a relative path (e.g. "data-fr/nazo/msg_start.cani"); on a hit fills
// the data's absolute offset + size and returns 1, else 0
int data0_locate(const char *rel, int64_t *abs_off, int64_t *size);

// read len bytes at an absolute offset (thread-safe); returns bytes read or -1
int data0_pread(void *buf, int64_t abs_off, int len);

// dup the pack's fd (for AAsset_openFileDescriptor64); returns fd or -1
int data0_dup_fd(void);

#endif
