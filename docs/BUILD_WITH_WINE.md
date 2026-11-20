# Building KAWAI連連看2 with Wine & Visual Basic 5.0 (Linux)

This guide covers building both **KAWAI連連看2 v1.02** (`src/`) and **v1.00-cn** (`src-llkcn/`) natively on Linux using **Wine** and **Microsoft Visual Basic 5.0 Enterprise Edition**.

Verified on **Arch Linux** with **Wine 11.22 (WoW64)** and `winetricks 20260125`.

---

## 1. Prerequisites

Install on your Linux host:

- `wine` and `winetricks`
- `python3` (Python 3.4+) and `pillow` (`pip install pillow` or your distro's `python3-pil`, required only if using `--bmp` or `tools/assets_mods/`)
- `git` (for applying patches cleanly)
- `libarchive-tools` (`bsdtar`) or `xorriso` (to extract `VB50ENT.ISO` — **`7z` cannot extract this ISO cleanly**)
- *(Optional, for MIDI BGM playback at runtime)* `fluidsynth` / `qsynth` + a General MIDI SoundFont (`.sf2`), or `timidity`

---

## 2. Prepare a Fresh Wine Prefix

On modern WoW64 Wine builds (Wine 11+), **do not** set `WINEARCH=win32` (WoW64 refuses `WINEARCH=win32`), and make sure the target `WINEPREFIX` directory exists before running `wineboot`:

```bash
export WINEPREFIX="$HOME/.wine-vb5"
mkdir -p "$WINEPREFIX"
wineboot --init
wineserver -w
```

Install the VB5 runtime, native `oleaut32`, and Core Fonts:

```bash
# Modern winetricks marks vb5run as broken on Wine 8.10+ (WineHQ bug #56209); pass --force
winetricks -q --force vb5run

# Native oleaut32 fixes a Wine builtin crash when compiling Form_Main.frx's .ico Icon;
# corefonts is required so native oleaut32 accepts Form_Table.frm's FontName = "Arial".
winetricks -q oleaut32 corefonts
```

> [!NOTE]
> `gdiplus` is **not** required. VB5 compiles the `.jpg` and `.gif` form assets and `LoadPicture` calls natively using `oleaut32`.

---

## 3. Install Visual Basic 5.0 Enterprise Edition

1. Download the **Microsoft Visual Basic 5.0 Enterprise** ISO from [WinWorldPC](https://winworldpc.com/product/microsoft-visual-bas/50).
2. Extract `VB50ENT.ISO` (do **not** use `7z`, which fails on this image with header errors):

   ```bash
   mkdir -p /tmp/vb5_cd
   bsdtar -xf VB50ENT.ISO -C /tmp/vb5_cd
   # Or: xorriso -osirrox on -indev VB50ENT.ISO -extract / /tmp/vb5_cd && chmod -R u+w /tmp/vb5_cd
   ```

3. Run the installer in Wine (launches 16-bit `winevdm.exe` + `ACMSETUP.EXE`) and follow the prompts using your CD key:

   ```bash
   wine /tmp/vb5_cd/VB/SETUP/SETUP.EXE
   ```

   *(Headless / minimal alternative: copy `/tmp/vb5_cd/VB/` to `$WINEPREFIX/drive_c/vb5/` and register the two `HKCR\Licenses` design-time license strings listed in `VB/SETUP/VB5STP.STF` via `wine reg add`.)*

---

## 4. Extract Original Game Assets

Place your verified original `D4s.exe` (`v1.02`) and/or `lianliankan2.ExE` (`v1.00-cn`) where convenient, then extract assets and build the `.frx` / `Project1.res` files:

```bash
# From the repository root:
python3 tools/extract_assets.py /path/to/D4s.exe --bmp
python3 tools/extract_assets.py /path/to/lianliankan2.ExE --bmp
```

- Without `--bmp`: extracts `.ico`, original `.jpg`/`.gif`, `.wav` (`201..206`), `.mid` (`101..103`), and packs `.frx` + `Project1.res` using pure Python stdlib.
- With `--bmp`: additionally writes 24-bit `.bmp` copies into `assets/images/` (using Pillow on Linux) while preserving the original `.jpg`/`.gif` timestamps so unmodified integrated builds keep exact compact `.frx` sizes until you modify a `.bmp`.

---

## 5. Apply Patches

### Required on Wine: `patches/wine-compat.patch`

Stock `Form_Main.frm` uses raw DBCS Big5 bytes inside `MsgBox` strings and `Timer_PlayMidi_Timer` (`"停止"`). Wine's ANSI parsing of VB5 source files can corrupt DBCS lead/trail bytes, so convert those 5 lines in each version to ASCII `ChrW$(&H...)` calls:

```bash
git apply patches/wine-compat.patch
# Or: python3 tools/apply_patch.py patches/wine-compat.patch
```

### Optional Patches (Stackable in Any Order)

- **Fix `v1.00-cn` bugs backported from `v1.02`:**

  ```bash
  git apply patches/llkcn-fix-midi-error70.patch patches/llkcn-fix-margin-crash.patch
  ```

- **External asset loading (`assets/` directory on disk, compact ~55 KB EXE):**

  ```bash
  git apply patches/external-assets.patch
  ```

*(To revert patches later: `git apply -R ...` or `python3 tools/apply_patch.py -R ...`)*

---

## 6. Build the Executables

### Option A: Helper Script (`tools/build_wine.sh`)

For **integrated standalone executables** (all images/audio compiled into the EXE):

```bash
WINEPREFIX="$HOME/.wine-vb5" ./tools/build_wine.sh --version all
# Outputs:
#   build/D4S.exe           (v1.02, ~861 KB)
#   build/Lianliankan2.exe  (v1.00-cn, ~861 KB)
```

For **external-asset executables** (after applying `patches/external-assets.patch`):

```bash
WINEPREFIX="$HOME/.wine-vb5" ./tools/build_wine.sh --version all --external
# Outputs right beside src/assets/ and src-llkcn/assets/:
#   src/D4S.exe             (~55 KB)
#   src-llkcn/Lianliankan2.exe (~55 KB)
```

### Option B: Manual `VB5.EXE /make` Invocation

```bash
VBP_WIN="$(winepath -w "$PWD/src/Project1.vbp")"
OUT_WIN="$(winepath -w "$PWD/D4S.exe")"
LOG_WIN="$(winepath -w "$PWD/build.log")"

LC_ALL=zh_TW.UTF-8 wine "$WINEPREFIX/drive_c/Program Files/DevStudio/VB/VB5.EXE" \
    /make "$VBP_WIN" "$OUT_WIN" /out "$LOG_WIN"
```

---

## 7. Running & MIDI Audio on Linux

Run the compiled executable under a Traditional Chinese locale (`v1.02` expects Big5 CP950 for ANSI menus/dialogs; `v1.00-cn` uses GBK `Form_Table` captions and Big5 gameplay UI strings):

```bash
# Integrated standalone EXE (works from any directory):
LC_ALL=zh_TW.UTF-8 WINEPREFIX="$HOME/.wine-vb5" wine build/D4S.exe

# External-assets EXE (must sit in the directory containing assets/):
(cd src && LC_ALL=zh_TW.UTF-8 WINEPREFIX="$HOME/.wine-vb5" wine ./D4S.exe)
```

For background MIDI music (`mciSendStringA` sequencer), start an ALSA MIDI synthesizer such as **Qsynth** / **FluidSynth** with a GM SoundFont (`*.sf2`) or `timidity -iA` before launching the game.

If menus or dialogs display square boxes (`□□(G)`) on a fresh prefix, install Traditional Chinese fonts into the prefix (`winetricks -q cjkfonts`) or install your distribution's CJK font package (e.g. `noto-fonts-cjk`).
