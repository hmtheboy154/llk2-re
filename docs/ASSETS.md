# Preparing Game Assets, OST, SoundFonts & Mod Packs

This repository contains **source code only** (`CC0-1.0`). No copyrighted original images, icons, sound effects, MIDI tracks, or commercial OST audio are distributed here.

To play **KAWAI連連看2 — Unofficial Remastered**, extract the assets locally from your own copy of the original freeware executable (`D4s.exe` v1.02 or `lianliankan2.ExE` v1.00-cn).

---

## 1. Supported Original Executables & SHA-256 Verification

Assets can be extracted from either of the two classic releases (including `.zip` archives containing either executable):
- [The official Traditional Chinese version v1.02](https://web.archive.org/web/20060423174908/http://myweb.hinet.net:80/home4/cps-chen/history/d4s/d4s.htm) (`D4s.exe`) made by [CHEN PROGRAM STUDY](https://web.archive.org/web/20070615052321/http://myweb.hinet.net/home4/cps-chen/main.htm).
- [The unofficial Simplified Chinese version v1.00-cn](http://www.llk.cn/LLK2/) (`lianliankan2.ExE`) made by Casm of [LLK.cn](http://www.llk.cn/) (Previously [www.LoveSnow.com](https://web.archive.org/web/20040729014651/http://www.lovesnow.com/)).

| Version | Filename | Expected SHA-256 |
|---|---|---|
| Official Traditional Chinese (`v1.02`) | `D4s.exe` | `72c8a9882badac35faf618e40b751d4c11d1f1cdf935fb94043c41afdba64c7a` |
| Unofficial Simplified Chinese (`v1.00-cn`) | `lianliankan2.ExE` | `d5306b7fdc3125259d49cee9054eaffebf67237f9240da82b244b932af7c919b` |

*(Both `llk2-launcher` and `tools/extract_assets.py` automatically verify the SHA-256 hash before extracting, unless you explicitly skip verification for a modified binary.)*

To verify manually from a terminal:

```bash
# Linux / macOS:
sha256sum D4s.exe lianliankan2.ExE
```

```powershell
# Windows (PowerShell):
Get-FileHash D4s.exe -Algorithm SHA256
Get-FileHash lianliankan2.ExE -Algorithm SHA256

# Windows (Command Prompt):
certutil -hashfile D4s.exe SHA256
```

---

## 2. Extracting Assets

### Option A: Using `llk2-launcher` (GUI — Recommended)

1. Start **`llk2-launcher`**.
2. Open the **Assets & Mods** tab (or simply **drag and drop** `D4s.exe`, `lianliankan2.ExE`, or a `.zip` archive onto the launcher window).
3. Confirm the green **SHA-256 Verified** message (`v1.02` or `v1.00-cn`).
4. Click **Extract to Default `assets/`**.
5. Switch back to the **Play** tab — you should see the Title Screen preview, the 42 composited 3D tiles, and **`Integrity Check PASSED: all 21 required assets present`**.

> [!NOTE]
> `llk2-launcher` invokes `tools/extract_assets.py` using Python 3 (`stdlib` only — no `pip` packages needed).

### Option B: Using the Command Line (`tools/extract_assets.py`)

`tools/extract_assets.py` unpacks the original ASPack-compressed executable completely in memory using pure Python 3 standard library modules (no `pip` packages or Wine required):

```bash
# Extract into ./assets/ (Portable / local dev folder):
python3 tools/extract_assets.py D4s.exe -o assets

# Or extract directly into your OS user data folder:
# Linux:
python3 tools/extract_assets.py D4s.exe -o ~/.local/share/llk2/assets

# Windows (PowerShell):
python tools\extract_assets.py D4s.exe -o "$env:APPDATA\llk2\assets"

# Extracting from a .zip archive works directly too:
python3 tools/extract_assets.py D4s.zip -o assets
```

Useful CLI flags:
- `-o, --out <dir>`: Destination directory (defaults to `<repo>/assets`).
- `--ost <ost_dir>`: Also import matching `01..03` / `101..103` `.flac`/`.wav`/`.mp3` OST tracks into `<out>/ost/`.
- `--skip-hash`: Allow extracting from a custom or modified EXE whose SHA-256 differs from stock `v1.02` / `v1.00-cn`.

---

## 3. Expected `assets/` Layout

A complete asset pack contains **21 required files** (plus optional `icons/`, `ost/`, and `locales/` overrides):

```text
assets/
├── icons/
│   └── Form_Main_Icon.ico
├── images/
│   ├── Pic_Title_Picture.{jpg,png,bmp}      # 500×375 title splash
│   ├── Pic_Data_Picture.{gif,png,bmp}       # HUD digits, timer bar, life icon, studio banner
│   ├── Pic_MjData_Picture.{jpg,png,bmp}     # 360×612 sprite sheet (42 tiles + masks + 6 tile shells)
│   ├── Form_Explain_Picture.{gif,png,bmp}   # 640×376 Game Rules graphic
│   ├── Form_Author_Picture.{jpg,png,bmp}    # 370×310 original Author card
│   └── Pic_Level_0..6_Picture.{jpg,png,bmp} # Seven 600×450 stage backgrounds
├── sfx/
│   └── 201.wav .. 206.wav                   # 6 sound effects
├── midi/
│   └── 101.mid .. 103.mid                   # 3 classic MIDI BGM tracks
├── ost/                                     # (Optional) Lossless/compressed OST audio
│   └── 101.{flac,wav,mp3} .. 103.{flac,wav,mp3}
└── locales/                                 # (Optional) Per-locale image overrides
    └── <lang>/Form_Explain_Picture.png
```

---

## 4. Where Assets, Configs & Mods Are Stored

All three game frontends (`llk2-qt`, `llk2-imgui`, `llk2-bui`) and `llk2-launcher` resolve the active data directory using the same priority order:

| Priority | Trigger | Active Data Directory |
|---|---|---|
| 1 | CLI flag `--data-dir <dir>` | `<dir>` |
| 2 | Environment variable `LLK2_DATA_DIR` | `$LLK2_DATA_DIR` |
| 3 | **Portable Mode** (`portable.txt` next to binary or `$APPIMAGE`, or a writable local dev/zip folder containing `assets/` or `config.json`) | That writable folder |
| 4 | **Standard OS User Directory** *(default for installed packages / AppImage)* | **Linux**: `~/.local/share/llk2` (`$XDG_DATA_HOME/llk2`)<br>**Windows**: `%APPDATA%\llk2`<br>**macOS**: `~/Library/Application Support/llk2` |

Inside the active data directory, everything lives together cleanly:
- `config.json` — display, audio, language, fullscreen settings
- `scores.json` — Top 10 UTF-8 leaderboard
- `assets/` — default extracted asset pack
- `mods/<pack_name>/` — custom mod packs (selectable in `llk2-launcher` or via `--assets <dir>`)
- `locales/` — optional user-added or overridden translation `.json` files

---

## 5. Optional: Lossless OST Audio

The three classic BGM tracks correspond to:
- **Track 1 (`101`)**: Michael P. Sirotkin — *Aquarelle*
- **Track 2 (`102`)**: Miyuki Nakajima — *Sora to Kimi no Aida ni*
- **Track 3 (`103`)**: Air Supply — *Making Love Out Of Nothing At All*

An archived soundtrack collection is available on Internet Archive ([KAWAI连连看 OST on archive.org](https://archive.org/details/0_pVh1BFkfw0AihZxQ7EIPAQ3YYA4X4BIQCLfLD8Q1bf)). If you have the OST files (`.flac`, `.wav`, or `.mp3`), place them in `<pack>/ost/` named either:
- `101.flac`, `102.flac`, `103.flac` *(or `.wav` / `.mp3`)*, **or**
- `01.<title>.flac`, `02.<title>.flac`, `03.<title>.flac`

You can also click **Import OST Folder / Audio Files...** in `llk2-launcher` → **Assets & Mods**, or pass `--ost <folder>` to `tools/extract_assets.py`. Then select **Lossless OST Audio (FLAC/MP3)** under **Settings → Music Playback Device**.

---

## 6. Optional: Custom SoundFont (`.sf2` / `.sf3`)

When building with submodules initialized, **GeneralUser-GS.sf2** is automatically copied next to the executables as `soundfont.sf2` for immediate out-of-the-box MIDI synthesis.

To use a different SoundFont:
- Choose **Settings → Music Playback Device → Select Custom SoundFont (.sf2)...** inside any game frontend, **or**
- Pick it on the **Settings** tab of `llk2-launcher`.

---

## 7. Creating Custom Mod Packs (`mods/`)

Mod packs live under `<data_dir>/mods/<pack_name>/` and follow the exact same folder layout as `assets/`.

### Using `llk2-launcher` Mod Pack Studio (Easiest)
1. Open **`llk2-launcher` → Assets & Mods**.
2. Click **New Mod Pack...** (clones your base `assets/` into `mods/<name>/`).
3. Customize any asset directly inside the launcher (no external image tools required):
   - **Tile Sprites (`#0–41`)**: Pick any PNG/JPG/BMP/WebP image. Transparent PNG alpha is preserved automatically; opaque images automatically key out the corner background color. The launcher writes a lossless 24-bit `Pic_MjData_Picture.bmp` with exact `32×32` centered sprite art and `SRCAND`/`SRCPAINT` silhouette masks while leaving the 3D tile shells intact.
   - **Stage Backgrounds (`Stage 1–7`)**: Automatically aspect-scaled and center-cropped to `600×450` (`Pic_Level_0..6_Picture.png`).
   - **Title Screen**: Automatically scaled and center-cropped to `500×375` (`Pic_Title_Picture.png`).
   - **SFX / MIDI**: Swap any of `sfx/201..206.wav` or `midi/101..103.mid`.
4. Click **Check Integrity**, switch to the **Play** tab to preview your modified title screen & 42-tile sheet, and click **Launch Game** (or run any frontend directly with `--assets <path-to-mod>`).

### Manual `Pic_MjData_Picture` Geometry Reference
If you prefer editing `Pic_MjData_Picture.bmp` / `.png` directly in GIMP/Photoshop:
- Canvas size: **`360 × 612`** (lossless `.bmp` or `.png` recommended so mask edges have no JPEG compression noise)
- Grid: `9 columns × 5 rows` of `40 × 50` cells (`TileId 0..41`: `col = id % 9`, `row = id / 9`)
- **Art cells** (`y = 0..249`): Colored sprite on pure black `RGB(0, 0, 0)` background inside the `32×32` flat face box (`x+4..x+35`, `y+9..y+40`)
- **Mask cells** (`y = 250..499`): Pure black `RGB(0, 0, 0)` sprite silhouette on pure white `RGB(255, 255, 255)` background
- **3D Tile Shells** (`y = 500..611`): 6 unselected palettes (`y = 500`), 6 selected palettes (`y = 556`), each `46 × 56`, plus the shell mask at `(276, 500)`.
