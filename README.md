<div align=center>

<img src="extras/banner.png" alt="Banner" width="40%">

</div>
<h1 align=center>Professor Layton: Lost Future HD — Nintendo Switch port</h1>

A wrapper/port of the Android release of **Professor Layton: Lost Future HD**
(a.k.a. *Unwound Future*, v1.0.3). It loads the original game binary
(`libll3.so`, arm64), resolves its imports against native Switch
implementations and patches it so it runs as if inside a minimal Android
environment.

## How to install

Create a folder for the game on your SD card, `/switch/layton3/`, and place:

1. `layton3_nx.nro`
2. `libll3.so` — extracted from the APK's `lib/arm64-v8a/` folder.
3. `assets/` — the **contents** of the APK's `assets/` folder (the `data`,
   `data0`, `data-en`, `data-de`, `data-fr`, `data-es`, `data-it`, `data-EU`,
   `data-en-EU`, … subfolders).

```
/switch/layton3/
  layton3_nx.nro
  libll3.so
  assets/
    data/
    data0/
    data-en/
    data-fr/
    data-EU/
    ...
```

Launch with a **game override** (hold R while starting a title) or a
forwarder

## Configuration

`config.txt` is created next to the `.nro` on first run:

* `screen_width` / `screen_height` — render resolution; `-1` picks
  1280×720 in handheld and 1920×1080 docked.
* `portrait` — `1` (default) renders the game in portrait, to be played with
  the console held rotated; `2` is the same portrait view rotated the other way
  (180°), for holding the console the opposite way up; `0` is landscape.

## Controls

* **Touchscreen** in handheld mode.
* With a controller, the **left stick** moves a hidden touch cursor and
  **A / R / ZR** taps it.

## Known limitations
* In landscape mode cutscenes always play fullscreen, so the movie player's
  rotate button only affects the on-screen controls there. Portrait mode
  keeps the original rotate behaviour.

## Build

devkitA64 plus these portlibs:

```
pacman -S switch-mesa switch-libdrm_nouveau switch-ffmpeg switch-dav1d \
          switch-bzip2 switch-zlib
```

## Credits

* **TheOfficialFloW** — for the original Android so-loader (gtasa_vita).
* **Rinnegatamante** — the Vita Layton: Lost Future port.
* **fgsfds** — the Switch so-loader groundwork reused here.

### Support

If you enjoy my work and want to support me :

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/D1D1P2MOG)

## Legal

No affiliation with Level-5. "Professor Layton" is a trademark of its owner.
This repository contains no assets or program code from the original game,
and none may be distributed with builds. Users must extract the required
files from their own legally obtained copy.

Source code is provided under the MIT License (see LICENSE).
