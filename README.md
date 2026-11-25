# KAWAI連連看2 — Unofficial Remastered

This branch (`remastered`) is a modern cross-platform C++20/SDL remaster of [KAWAI連連看2](https://web.archive.org/web/20060423174908/http://myweb.hinet.net:80/home4/cps-chen/history/d4s/d4s.htm), built from the decompiled VB5 source code with the help of AI.

> [!NOTE]
> Looking for the original 1999 Visual Basic 5 decompilation (`D4s.exe` v1.02 & `lianliankan2.ExE` v1.00-cn)? Switch to the [`main`](../../tree/main) branch.

---

## Project Status & Highlights

The remaster is fully playable, cross-platform, and aims to replicate the original `v1.02` gameplay, timing, board shifts, and scoring as closely as possible while adding modern quality-of-life upgrades. Any bug reports, accuracy feedback, translations, or PRs are appreciated!

Further info from AI:

- **Faithful Original Gameplay Parity (`libllk2_core`)**: Reproduces the exact `18×11` board (`16×9 = 144` tiles), 42 tile sprites (`36` match classes), `CheckLink0..3` (≤ 2 turns), all 7 stages & 6 gravity shifts (`Static`, `Shift Up/Down/Left/Right`, `Split Left/Right`, `Gather Top/Bottom`), authentic VB5 `Rnd` LCG (`rtcRandomNext`), `200 ms` 3-pass lightning flash, `v1.02` scoring, pixel-exact `SRCAND`/`SRCPAINT` compositing, and 120-byte legacy `d4s.hst` compatibility.
- **3 Native UI Frontends + Standalone Launcher**:
  - `llk2-qt` — primary native desktop UI (**Qt 6** + SDL3/SDL2)
  - `llk2-imgui` — lightweight SDL3/SDL2 + **Dear ImGui** + FreeType UI
  - `llk2-bui` — optional **BUI** Blender-themed UI (Dark/Light themes, isolated `GPL-3.0-or-later`)
  - `llk2-launcher` — standalone Qt 6 launcher & **Mod Pack Studio** (kept cleanly separated from the game executables)
- **New Gameplay & Display Features**: **Endless Mode** (`All Levels` with bag-shuffle or `Specific Level(s) Loop`, plus optional `No Time Limit` and `Unlimited Shuffle` — never overwrites High Scores), **Borderless Fullscreen** (`F11` / `Alt+Enter`), `1×–3×` & aspect-locked **Free Resize** with Nearest/Bilinear filters, and a spoiler-safe **`F12` Inspector / Hint Assistant**.
- **Modern Audio Engine**: Switch live between **Embedded SoundFont Synth** (`TinySoundFont` + bundled `GeneralUser-GS.sf2` or any custom `.sf2`), **Native OS MIDI** (ALSA / WinMM), and **Lossless OST Audio** (`FLAC`/`WAV`/`MP3`), with individual track toggles (`101–103`) and lock-free WAV SFX mixing.
- **Multi-Language Support**: Built-in **Traditional Chinese (`zh-TW`)**, **Simplified Chinese (`zh-CN`)**, **English (`en`)**, and **Vietnamese (`vi`)** with automatic OS locale detection on first launch (falling back to English), live switching, localized **Game Rules** overlays, redesigned dark **About** screen with clickable credits, and runtime `locales/*.json` extensibility.
- **Cross-Platform Verified**: Built and tested cleanly (**8/8 CTest passing**) on both **Arch Linux** (GCC 16, SDL3, Qt 6) and **Windows 11 Pro 25H2** (MSYS2 UCRT64, GCC 16, SDL3, Qt 6).

For a full breakdown of everything new and changed compared to the original 1999 VB5 release, see [docs/FEATURES.md](docs/FEATURES.md).

---

## Quick Start & Preparing Assets

No copyrighted game assets (sprites, backgrounds, SFX, MIDIs, or OST audio) are shipped in this repository. You must provide your own copy of the original `D4s.exe` (`v1.02`) or `lianliankan2.ExE` (`v1.00-cn`) and extract assets locally.

| Version | File | Expected SHA-256 |
|---|---|---|
| Official `v1.02` | `D4s.exe` | `72c8a9882badac35faf618e40b751d4c11d1f1cdf935fb94043c41afdba64c7a` |
| Unofficial `v1.00-cn` | `lianliankan2.ExE` | `d5306b7fdc3125259d49cee9054eaffebf67237f9240da82b244b932af7c919b` |

Extract assets either by **dragging and dropping** the EXE (or `.zip` archive) into **`llk2-launcher` → Assets & Mods**, or from the command line using pure Python 3 (`stdlib` only):

```bash
python3 tools/extract_assets.py D4s.exe -o assets
```

See **[docs/ASSETS.md](docs/ASSETS.md)** for full instructions on SHA-256 verification, optional OST audio, custom `.sf2` SoundFonts, and creating Mod Packs.

---

## Building

Clone the `remastered` branch with submodules:

```bash
git clone --recursive -b remastered https://github.com/hmtheboy154/llk2-re.git
cd llk2-re
```

### Platform Build Guides
- **Linux** (Arch, Debian/Ubuntu, Fedora): **[docs/BUILD_FOR_LINUX.md](docs/BUILD_FOR_LINUX.md)**
- **Windows** (MSYS2 `UCRT64` + standalone `dist/` packaging): **[docs/BUILD_FOR_WINDOWS.md](docs/BUILD_FOR_WINDOWS.md)**

### CMake Targets & Options

| Option / Target | Default | Description |
|---|---|---|
| `llk2_core`, `llk2_media` | Always built | Pure C++20 game logic + SDL3/SDL2 renderer & audio libraries |
| `LLK2_UI_QT` → `llk2-qt` | `ON` | Primary Qt 6 desktop executable |
| `LLK2_UI_IMGUI` → `llk2-imgui` | `ON` *(if submodule present)* | SDL + Dear ImGui + FreeType executable |
| `LLK2_UI_BUI` → `llk2-bui` | `ON` *(if submodule present)* | Optional Blender-style BUI executable (`GPL-3.0-or-later` isolated) |
| `LLK2_LAUNCHER` → `llk2-launcher` | `ON` | Standalone Qt 6 launcher & Mod Pack Studio |
| `LLK2_SDL_VERSION` | `AUTO` | Prefer `SDL3` if found, otherwise `SDL2` (`3` or `2` to force) |
| `LLK2_BUILD_TESTS` | `ON` | 8-target headless `ctest` suite (`ctest --test-dir build --output-on-failure`) |

---

## Controls, Storage & CLI Flags

### Controls & Shortcuts
- **Left Click**: Select a tile / match two identical tiles with ≤ 2 turns
- **Right Click**: Cancel current selection
- **`F11` / `Alt+Enter`**: Toggle Borderless Fullscreen
- **`F12`**: Toggle Inspector / Hint Assistant window
- **`Esc`**: Close active modal dialog

### Unified Storage Locations & Portable Mode
All game frontends and `llk2-launcher` share a single unified data folder (`config.json`, `scores.json`, `assets/`, `mods/`, `locales/`):

- **Linux**: `$XDG_DATA_HOME/llk2` *(defaults to `~/.local/share/llk2`)*
- **Windows**: `%APPDATA%\llk2` *(no longer writes to `C:\Windows\d4s.hst`)*
- **macOS**: `~/Library/Application Support/llk2`
- **Portable Mode**: Place an empty `portable.txt` beside the executables (or run inside a writable folder that already contains `assets/` or `config.json`) to store everything locally.

### CLI Flags & Environment Variables
- `--assets <dir>` — load a specific asset pack or mod directory
- `--data-dir <dir>` or `LLK2_DATA_DIR=<dir>` — override the active data directory

---

## Modding & Translations

- **Mod Pack Studio (`mods/`)**: Open **`llk2-launcher` → Assets & Mods** to clone `assets/` into `mods/<pack>/`, replace individual tile sprites (`#0–41`, auto-generating lossless `Pic_MjData_Picture.bmp` art & `SRCAND`/`SRCPAINT` masks), swap `600×450` stage backgrounds, title screen, SFX, MIDIs, or OST tracks, and run the 21-file Integrity Checker. See [docs/ASSETS.md](docs/ASSETS.md).
- **Custom Translations (`locales/`)**: Drop any `<lang>.json` file (modeled after `locales/en.json`) into `<data_dir>/locales/` — it will appear automatically in the Language menu across all frontends and the launcher. Optional per-locale Game Rules images can be placed at `assets/locales/<lang>/Form_Explain_Picture.png`.

---

## License & Third-Party Libraries

All original code authored in this repository (`libllk2_core`, `libllk2_media`, `llk2-qt`, `llk2-imgui`, `llk2-launcher`, `locales/`, `tools/`, `tests/`) is released under **[CC0-1.0 Universal](LICENSE)** (Public Domain).

> [!IMPORTANT]
> The optional **`llk2-bui`** executable links against the [`subprojects/BUI`](https://github.com/ppmpreetham/BUI) submodule, which is licensed under **GPL-3.0-or-later**. It is strictly isolated into its own separate executable target (`-DLLK2_UI_BUI=OFF` excludes it completely so all remaining binaries stay free of GPL obligations).

| Component | Repository / Source | License |
|---|---|---|
| [TinySoundFont](https://github.com/schellingb/TinySoundFont) (`tsf.h`, `tml.h`) | `subprojects/TinySoundFont` | MIT |
| [stb](https://github.com/nothings/stb) (`stb_image.h`, `stb_image_write.h`) | `subprojects/stb` | Public Domain / MIT |
| [dr_libs](https://github.com/mackron/dr_libs) (`dr_flac.h`, `dr_mp3.h`) | `subprojects/dr_libs` | Public Domain / MIT-0 |
| [GeneralUser GS](https://github.com/mrbumpy409/GeneralUser-GS) | `subprojects/GeneralUser-GS` | [GeneralUser GS License](https://schristiancollins.com/generaluser.php) |
| [Dear ImGui](https://github.com/ocornut/imgui) | `subprojects/imgui` | MIT |
| [BUI](https://github.com/ppmpreetham/BUI) *(optional `llk2-bui` only)* | `subprojects/BUI` | GPL-3.0-or-later |
| [Qt 6](https://www.qt.io/), [SDL](https://github.com/libsdl-org/SDL), [FreeType](https://freetype.org/) | System / MSYS2 libraries | LGPL-3.0 / zlib / FTL |

---

## Credit

- Special thanks to [CHEN PROGRAM STUDY](https://web.archive.org/web/20070615052321/http://myweb.hinet.net/home4/cps-chen/main.htm) for creating the game that is a part of my childhood :).
- Casm of [LLK.cn](http://www.llk.cn/) (Previously [www.LoveSnow.com](https://web.archive.org/web/20040729014651/http://www.lovesnow.com/)) for the classic Simplified Chinese translation (`v1.00-cn`).
- Shout out to all the open-source projects listed in [License & Third-Party Libraries](#license--third-party-libraries) above.
