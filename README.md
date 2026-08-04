<p align="center">
  <img src="https://raw.githubusercontent.com/SSOROZCO/files/refs/heads/main/Strawberry-Roon.svg" width="220" alt="Strawberry-Roon">
</p>

<h1 align="center">Strawberry Astra - Roon Edition</h1>

<p align="center">
A Roon-inspired, audiophile visual refresh for Strawberry Music Player — Astra SE ready
</p>

<p align="center"></p>

[![Build Status](https://github.com/SSOROZCO/Strawberry-Astra/actions/workflows/build.yaml/badge.svg?branch=master)](https://github.com/SSOROZCO/Strawberry-Astra/actions)

[![Sponsor](https://img.shields.io/badge/-Sponsor-green?logo=github)](https://github.com/sponsors/SSOROZCO)
[![Patreon](https://img.shields.io/badge/patreon-donate-green.svg)](https://www.patreon.com/cw/SergioSebastianOrozco)
[![PayPal](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/cineflix)

**Base:** Strawberry 1.2.26 official | **Current:** 1.2.26-r2 FINAL TRUE | **Binary:** `strawberry` | **Theme:** Roon #101114 + #C1272D

Inspired by Roon's editorial layout and the restrained, dark palette of Astra Linux Special Edition, Strawberry Astra modernizes Strawberry's appearance while preserving 100% of its engine, performance and audiophile features.

Originally forked from Clementine in 2018, Strawberry is developed in C++ / Qt 6 / GStreamer. Astra does not remove features or imitate other players — it delivers a cleaner, more contemporary, listening-room identity.

![Screenshot of Strawberry Music Player](https://raw.githubusercontent.com/SSOROZCO/files/refs/heads/main/Screenshot_20260803_231109.png)

---
## Project Vision

The initiative began with a simple idea: to refresh Strawberry's interface without altering the qualities that make it exceptional. While early inspiration came from Roon's clean layout, the final design direction was guided by the minimalist environment of Astra Linux Special Edition.

### Objectives
- Modernise the interface while maintaining simplicity and speed
- Preserve bit-perfect playback, lightweight performance and stability
- Ensure functional consistency across all elements
- Refine visual coherence with updated icons and Roon analyzers
- Respect the original project and its philosophy — **The UI changes. The engine does not.**

### Design Philosophy
- Keep Strawberry fast, lightweight and stable
- Roon Black #101114 surfaces with Roon Red #C1272D accent
- More space, better rhythm, real typography hierarchy
- Blend naturally with modern dark desktops
- Honour Strawberry upstream

## ✨ What’s New in Astra R2 FINAL TRUE (1.2.26-r2)

This is not just icons. R2 is a full Roon Edition:

**Visual System:**
- Full Roon QSS theme `data/roon.qss` — dark #101114, red accent #C1272D, rounded cards, slim scrollbars, pill-style selection
- New minimalist icon set (220px SVG source), adapted to Roon style
- Larger artwork in Now Playing (300px+), rounded corners, editorial metadata hierarchy
- Sidebar reworked: glossy gradient removed, neutral panel + rounded pill active state
- Improved spacing and padding across queue, playlist and player controls
- More intuitive layout — clearer grouping, less clutter

**Audio Intelligence UI:**
- **Hi-Res badge:** Now Playing shows if track is Hi-Res (FLAC 24/96+, DSD, etc.)
- **DAC detection:** Shows detected DAC / output device info in context
- **Roon Analyzer:** New `roonanalyzer.cpp` — Roon-style spectrum with #C1272D
- **Dorrough Analyzer:** `dorroughanalyzer.cpp` — professional loudness metering
- **Rainbow / Turbine / Boom analyzers** refined with Roon palette

**Engineering:**
- Version system fixed: follows official cycle `-r1`, `-r2` — now `1.2.26-r2` everywhere (no more hash `9b6d4a7a`)
- Binary name kept as `strawberry` to preserve config `~/.config/strawberry`, collection DB, MPRIS `org.mpris.MediaPlayer2.strawberry`
- Astra Linux SE integration tested
- Clean build: `build/` ignored via `.gitignore`, reproducible .deb

**Identity:**
- Window title, About dialog and launcher show *Strawberry Astra - Roon Edition*
- Internal IDs unchanged for config continuity

## :white_check_mark: Features (100% Strawberry preserved)

- Play and organize your music collection
- Support for WAV, FLAC, Ogg FLAC, WavPack, Ogg Vorbis, Opus, Ogg Speex, MPC, TrueAudio, AIFF, MP4/AAC, ALAC, MP3, ASF, Monkey’s Audio, and DSD (DSF/DSDIFF)
- Bit-perfect playback on Linux
- MPRIS2 / D-Bus remote control on Linux
- Native desktop notifications
- Advanced playlist management, smart and dynamic playlists
- Audio analyzer, equalizer, moodbar, and waveform seek bar — now with Roon & Dorrough analyzers
- Hi-Res detection and DAC info display (Astra exclusive UI)
- Volume normalization with ReplayGain and EBU R128 loudness analysis
- Editing tags, and fetching missing tags via acoustic fingerprinting using [AcoustID](https://acoustid.org/) and [MusicBrainz](https://musicbrainz.org/)
- Album cover art from: [Last.fm](https://www.last.fm/), [MusicBrainz](https://musicbrainz.org/), [Discogs](https://www.discogs.com/), [Musixmatch](https://www.musixmatch.com/), [Deezer](https://www.deezer.com/), [Tidal](https://www.tidal.com/), [Qobuz](https://www.qobuz.com/), [Spotify](https://www.spotify.com/)
- Lyrics from: [Genius](https://genius.com/), [Musixmatch](https://www.musixmatch.com/), [lyrics.ovh](https://lyrics.ovh/), [songlyrics](https://www.songlyrics.com/), [azlyrics](https://www.azlyrics.com/), [elyrics](https://www.elyrics.net/), [letras](https://www.letras.mus.br) and [lrclib.net](https://lrclib.net/)
- Audio format conversion (transcoding) to MP3, AAC, FLAC, Ogg Vorbis, Opus, Speex, WavPack, and ASF
- Music transfer to USB, MTP and iPod devices
- Scrobbling to [Last.fm](https://www.last.fm/), [ListenBrainz](https://listenbrainz.org/), and Subsonic
- Global keyboard shortcuts (Linux, macOS, and Windows)
- Discord Rich Presence
- Audio CD playback
- Internet radio from [Radio Paradise](https://radioparadise.com/), [SomaFM](https://somafm.com/), [Radio Browser](https://www.radio-browser.info/), and custom streams
- Streaming from Subsonic-compatible servers
- Unofficial Tidal, Spotify, and Qobuz integration

---
:white_check_mark: Tested on **Linux / Astra Linux Special Edition**.

---

## :gear: Requirements

To build Strawberry Astra from source, you’ll need:

**Dependencies:**
- [CMake 3.13 or higher](https://cmake.org/)
- C/C++ compiler ([GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), or [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/))
- [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) or [pkgconf](https://github.com/pkgconf/pkgconf)
- [Boost](https://www.boost.org/)
- [GLib](https://developer.gnome.org/glib/)
- [Qt 6.4 or higher](https://www.qt.io/) (Core, Concurrent, Gui, Widgets, Network, SQL, D-Bus)
- [SQLite 3.9 or higher](https://www.sqlite.org)
- [ALSA (Linux only)](https://www.alsa-project.org/)
- [GStreamer](https://gstreamer.freedesktop.org/)
- [TagLib 1.12 or higher](https://www.taglib.org/)
- [ICU](https://unicode-org.github.io/icu/)
- [KDSingleApplication 1.1.0 or higher](https://github.com/KDAB/KDSingleApplication)

**Optional:**
- Chromaprint, FFTW3, PulseAudio, libcdio, libmtp, libgpod, libebur128
- GStreamer plugins base, good, and optionally bad, ugly and libav

---

## 🔧 Build from Source

```bash
git clone --recursive https://github.com/SSOROZCO/Strawberry-Astra.git
cd Strawberry-Astra
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel $(nproc)
./build/strawberry --version
# Strawberry 1.2.26-r2
sudo cmake --install build
