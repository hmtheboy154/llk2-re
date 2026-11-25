# Features & Comparison (Remastered vs Original 1999 VB5)

This document summarizes what the **Unofficial Remastered (`remastered`)** edition preserves from the original 1999 **KAWAI連連看2** (`D4s.exe` v1.02 / `lianliankan2.ExE` v1.00-cn) and everything new or improved in the modern C++20/SDL port.

---

## Side-by-Side Overview

| Category | Original 1999 VB5 (`D4s.exe` / `v1.00-cn`) | Unofficial Remastered (`remastered`) |
|---|---|---|
| **Platforms** | Windows 95/98/XP (32-bit VB5 + Win32 GDI) | **Linux, Windows, macOS** (native 64-bit C++20 + SDL3/SDL2) |
| **UI Frontends** | Single fixed VB5 Win32 form | **3 separate frontends** (`llk2-qt`, `llk2-imgui`, `llk2-bui`) + standalone **`llk2-launcher`** |
| **Game Modes** | Classic 7-stage campaign only | Classic 7-stage campaign + **Endless Mode** (All Levels / Specific Level(s) Loop, bag shuffle, No Time Limit, Unlimited Shuffle) |
| **Display & Scaling** | Fixed `768×550` window only | **`1×`, `1.5×`, `2×`, `3×`, Free Resize** (keep aspect), **Nearest / Bilinear** filters, **Borderless Fullscreen** (`F11` / `Alt+Enter`) |
| **Languages** | Hardcoded Big5 (`v1.02`) or GBK (`v1.00-cn`) | Built-in **`zh-TW`, `zh-CN`, `en`, `vi`** with live switching, localized **Game Rules** overlays, and extensible `locales/*.json` |
| **Music & Audio** | Win32 MCI MIDI + `PlaySound` WAV (no track selection; Error 70 loop bug in `v1.00-cn`) | **3 playback backends** (Embedded `GeneralUser-GS` SoundFont synth, Native OS MIDI, Lossless OST FLAC/WAV/MP3), custom `.sf2`, per-track toggles (`101–103`), lock-free WAV SFX mixer |
| **High Scores** | `C:\Windows\d4s.hst` (120-byte fixed 8-byte ANSI names) | Modern UTF-8 **`scores.json`** (full Unicode names) + exact 120-byte legacy **`d4s.hst`** Import/Export |
| **Storage Layout** | Writes directly to `C:\Windows\` & temp folder | Unified user folder (`~/.local/share/llk2`, `%APPDATA%\llk2`, `~/Library/Application Support/llk2`) + automatic **Portable Mode** (`portable.txt`) |
| **Inspection / Hints** | None | Non-modal **`F12` Inspector / Hint Assistant** (hint coordinates hidden by default until board highlight is enabled) |
| **Modding & Assets** | Packed inside EXE (ASPack + VB5 FRX/PE) | External `assets/` & `mods/<pack>/` packs managed via **`llk2-launcher`** Mod Studio (tile/mask & background generator, integrity checker) |

---

## 1. Faithful Original Gameplay Parity (`libllk2_core`)

The core game engine (`libllk2_core`) is written in pure C++20 with zero UI dependencies and verified by automated `ctest` unit tests against the decompiled VB5 v1.02 behavior:

- **Board & Tiles**: Exact `18×11` logical grid (`16×9 = 144` playable tiles surrounded by a 1-cell empty outer border), 42 tile sprites (`34` standard pairs × 4 copies + 4 flower tiles + 4 season tiles = 36 match classes).
- **Pathfinding (`CheckLink0..3`)**: Exact matching rule allowing up to **2 right-angle turns** (at most 3 straight segments) through empty cells or the outer border ring.
- **All 7 Classic Stages & 6 Gravity Shifts**:
  1. Stage 1 — Static (no shift)
  2. Stage 2 — Shift Up
  3. Stage 3 — Shift Down
  4. Stage 4 — Shift Left
  5. Stage 5 — Shift Right
  6. Stage 6 — Split Left / Right
  7. Stage 7 — Gather Top / Bottom
- **RNG, Timing & Scoring**:
  - Authentic VB5 `Rnd` linear congruential generator (`rtcRandomNext`) seeded from wall-clock time (`Randomize Timer`).
  - `600`-second stage timer, `10` starting auto-shuffles (`+1` per cleared stage), `200 ms` 3-pass lightning beam animation (`White -> Red -> Yellow`), and exact `v1.02` scoring (`+20` per matched pair, remaining seconds bonus per stage, `lives × 50` on all-clear or timeout).
  - **Responsive rapid clicking** with gravity-shift protection (clicking the next tile during the 200 ms flash immediately completes the previous pair, while canceling accidental clicks if a gravity shift or deadlock shuffle moved a different tile under your cursor).
  - Includes the `v1.02` bug fixes for right/bottom board-margin clicks and clean MIDI handle management.
- **Pixel-Exact Compositing (`libllk2_media`)**: Reproduces Win32 `BitBlt` `SRCAND` + `SRCPAINT` compositing for all 3D bevel tile shells (6 color palettes), tile sprites, HUD numbers, timer gradient bar, and hover/selection frames at native `768×550`.

---

## 2. New Gameplay Mode: Endless Mode

Open **`Game -> Endless Mode...`** in any frontend to start an Endless session:

- **Never overwrites High Scores**: Endless runs are strictly for relaxed play or stage practice and never save to `scores.json` or trigger leaderboard name entry.
- **Two stage selection modes**:
  1. **All Levels (Stages 1–7)**: Loops endlessly across all 7 shift patterns, with an optional **Shuffle stage order** checkbox using bag-shuffling (every stage appears once per bag and never repeats back-to-back across bag boundaries).
  2. **Specific Level(s) Loop**: Pick any single stage or custom subset of Stages 1–7 to loop endlessly (shuffle becomes available whenever 2 or more stages are checked).
- **Optional Relaxed Rules** *(off by default — classic timer & lives apply unless checked)*:
  - **No Time Limit (Relaxed)**: Freezes the stage timer at full and removes timeouts.
  - **Unlimited Shuffle**: Prevents deadlock auto-shuffles from deducting lives or triggering "No More Moves" Game Over.
- **Progress Tracking**: Stage-clear dialogs and the `F12` Inspector display your Endless run stage number (`#1`, `#2`, ...) and active shift rule.

---

## 3. Display, Window Scaling & Borderless Fullscreen

- **Window Scale Presets**: Switch live between `1× (768×550 Native)`, `1.5× (1152×825)`, `2× (1536×1100)`, `3× (2304×1650)`, and **`Free Resize (Keep Aspect)`** with clean black letterboxing.
- **Scaling Filter**: Choose **Crisp Pixels (Nearest)** for sharp retro pixels or **Smooth (Bilinear)** for softer scaling.
- **Borderless Fullscreen**: Toggle via **`Settings -> Borderless Fullscreen`**, **`F11`**, or **`Alt+Enter`**. Persisted independently from your windowed scale mode in `config.json`, so exiting fullscreen always restores your previous window size.

---

## 4. Three Independent UI Frontends

All three game executables share the exact same `libllk2_core` engine, `libllk2_media` renderer/audio stack, settings, and high scores, and can be launched directly without the launcher:

| Executable | Toolkit | Highlights | License |
|---|---|---|---|
| **`llk2-qt`** | Qt 6 Widgets + SDL3/SDL2 | Primary desktop UI with native OS menu bar, dialogs, and automatic CJK/Vietnamese font fallback | `CC0-1.0` |
| **`llk2-imgui`** | SDL3/SDL2 + Dear ImGui + FreeType | Lightweight single-window UI with crisp FreeType Latin/Vietnamese + CJK merged font atlas | `CC0-1.0` |
| **`llk2-bui`** | BUI (`bui::kit`) | Pure Blender-style UI with switchable **Blender Dark / Light** themes, Blender File Browser, and tool-window Inspector *(optional build target)* | `GPL-3.0-or-later` *(isolated)* |

---

## 5. Standalone Launcher & Mod Studio (`llk2-launcher`)

A dedicated Qt 6 launcher (`CC0-1.0`) kept separate from the game binaries so the games themselves stay clean and fast while pre-launch setup and modding live in one place:

- **Play Tab**: Auto-detects installed frontends (`llk2-qt`, `llk2-imgui`, `llk2-bui`), lets you pick `Default (assets/)`, any mod pack in `mods/`, or a custom folder, displays live **Title Screen** and **composited 42-tile 3D-shell** previews with integrity status, and launches with `--data-dir` and `--assets`.
- **Assets & Mods Tab**:
  - **Local EXE / ZIP Extractor**: Drag-and-drop or browse for user-supplied `D4s.exe`, `lianliankan2.ExE`, or a `.zip` containing it, verify its **SHA-256** version (`v1.02` / `v1.00-cn`), and extract with live log output.
  - **OST Importer**: One-click import of local `01..03` / `101..103` `.flac`/`.wav`/`.mp3` OST folders into `<pack>/ost/`.
  - **Mod Pack Studio**: Create, duplicate, delete, and integrity-check mod packs under `mods/`; replace individual **Tile Sprites (#0–41)** via a built-in lossless C++/Qt `Pic_MjData` sprite + `SRCAND`/`SRCPAINT` mask generator (zero Python Pillow dependency), replace **Stage Backgrounds (`600×450`)**, **Title Screen (`500×375`)**, **SFX (`201–206.wav`)**, and **MIDIs (`101–103.mid`)**.
- **Settings Tab**: Live editor for shared `config.json` with instant launcher UI re-translation.
- **High Scores Tab**: Edit player names, clear the leaderboard, import/export legacy `d4s.hst`, and backup/restore `scores.json`.
- **Diagnostics Tab**: Shows active storage mode (*User Data Directory* vs *Portable Mode*), resolved paths, detected binaries/SoundFont/OST health, and one-click folder openers.

---

## 6. Audio & Music Backends

- **Embedded SoundFont Synth** *(default)*: Uses `TinySoundFont` + `TinyMidiLoader` with bundled **GeneralUser-GS.sf2** (plus support for selecting any custom `.sf2` / `.sf3` file), giving rich General MIDI playback on every OS with zero driver setup.
- **Native OS MIDI Device**: Routes MIDI playback to ALSA Sequencer on Linux or WinMM on Windows.
- **Lossless OST Audio**: Streams optional user-supplied `FLAC`, `WAV`, or `MP3` tracks (`assets/ost/101..103.*`) via `dr_flac` / `dr_mp3`.
- **Per-Track Toggles**: Enable or disable any combination of Track 1 (`101`), Track 2 (`102`), and Track 3 (`103`) individually.
- **Zero-Latency SFX**: Lock-free ring-buffered WAV mixer plays all 6 classic sound effects (`201–206.wav`) cleanly alongside background music.

---

## 7. Localization, Game Rules Overlays & Redesigned About Screen

- **4 Built-In Languages**: **Traditional Chinese (`zh-TW`)**, **Simplified Chinese (`zh-CN`)**, **English (`en`)**, and **Vietnamese (`vi`)** — embedded directly into the binaries from `locales/*.json` at build time, with **automatic OS locale detection** on first launch (Windows `GetUserDefaultLocaleName` / Linux `LANGUAGE`, `LC_ALL`, `LC_MESSAGES`, `LANG`, falling back to English for unsupported locales), runtime `locales/*.json` override/addition support, and instant live switching.
- **Localized Game Rules**: Preserves the original `Form_Explain_Picture` tile diagrams and red frame while rendering localized rules text and controls for `en` and `vi` (and supports drop-in per-locale `assets/locales/<lang>/Form_Explain_Picture.png` overrides).
- **Redesigned About Screen**: Unified `684×310` dark card displaying the original `Form_Author_Picture` on the left alongside center-aligned colored credits and clickable external hyperlinks on the right.

---

## 8. `F12` Inspector / Hint Assistant

Press **`F12`** (or toggle **`Settings -> Inspector / Hint Assistant`**) for a non-modal live status window showing:
- Current mode (`Title Screen` / `Playing` / `Endless`), stage & gravity shift rule, remaining tiles (`N / 144`), lives/score, remaining time (`Unlimited` when No Time Limit is active), audio backend, active SoundFont, and currently playing track.
- **Spoiler-Safe Hint Assistant**: Valid-pair coordinates stay **Hidden** when you open the Inspector; checking **"Highlight Hint Pair on Board"** reveals the coordinates and draws a golden highlight around a valid pair on the board.

---

## 9. Cross-Platform Storage & Legacy `d4s.hst` Compatibility

- **Unified User Data Folder**:
  - **Linux**: `$XDG_DATA_HOME/llk2` *(defaults to `~/.local/share/llk2`)*
  - **Windows**: `%APPDATA%\llk2`
  - **macOS**: `~/Library/Application Support/llk2`
- **Portable Mode**: Place an empty `portable.txt` next to the executables (or keep `assets/` / `config.json` in a writable local directory) to keep all settings, scores, assets, and mods self-contained.
- **UTF-8 `scores.json` + Legacy `d4s.hst`**: Full Unicode player names are saved in `scores.json`, while exact 120-byte `d4s.hst` Import/Export lets you migrate scores to or from the original 1999 Windows game.
