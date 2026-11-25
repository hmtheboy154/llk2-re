# Building on Windows (MSYS2 UCRT64)

This guide covers building **KAWAI連連看2 — Unofficial Remastered** (`llk2-qt.exe`, `llk2-imgui.exe`, `llk2-bui.exe`, and `llk2-launcher.exe`) natively on Windows using **MSYS2 (`UCRT64`)** — no Visual Studio / MSVC installation required.

*(Verified on **Windows 11 Pro 25H2** with MSYS2 UCRT64, GCC 16.2, CMake 4.5, Ninja, SDL 3.4, Qt 6.12, FreeType 2.15, and Python 3.14 — all 8 CTest targets and standalone `dist/` deployment passing.)*

---

## 1. Install MSYS2

1. Download and install **MSYS2** from [https://www.msys2.org/](https://www.msys2.org/) into the default `C:\msys64` folder.
2. Open **MSYS2 UCRT64** from the Start Menu *(important: use **UCRT64**, not the plain MSYS shell)*.
3. Update MSYS2 core packages (if prompted to close the terminal after updating, reopen **MSYS2 UCRT64** and run it once more):

```bash
pacman -Syu
```

---

## 2. Install Build Dependencies

Inside the **MSYS2 UCRT64** shell, install Git, GCC, CMake, Ninja, SDL3, Qt 6, FreeType, and Python 3:

```bash
pacman -S --needed \
    git \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-sdl3 \
    mingw-w64-ucrt-x86_64-qt6-base \
    mingw-w64-ucrt-x86_64-freetype \
    mingw-w64-ucrt-x86_64-python
```

*(If you prefer SDL2 over SDL3, install `mingw-w64-ucrt-x86_64-SDL2` and pass `-DLLK2_SDL_VERSION=2` to CMake.)*

---

## 3. Clone the Repository (with Submodules)

```bash
git clone --recursive -b remastered https://github.com/hmtheboy154/llk2-re.git
cd llk2-re
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

---

## 4. Configure & Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Optional Build Flags

- To build **only** `CC0-1.0` targets (excluding the optional `GPL-3.0-or-later` `llk2-bui.exe` frontend):
  ```bash
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLK2_UI_BUI=OFF
  cmake --build build
  ```
- To skip tests: pass `-DLLK2_BUILD_TESTS=OFF`.

When `subprojects/GeneralUser-GS` is initialized, CMake automatically copies `GeneralUser-GS.sf2` into `build/soundfont.sf2` next to the executables.

---

## 5. Extract Original Game Assets

See [ASSETS.md](ASSETS.md) for expected SHA-256 hashes and details.

From **MSYS2 UCRT64** (or after opening `llk2-launcher.exe`):

```bash
# Local portable checkout folder:
python tools/extract_assets.py /c/path/to/D4s.exe -o assets

# Or unified Windows user data folder (%APPDATA%\llk2\assets):
python tools/extract_assets.py /c/path/to/D4s.exe -o "$(cygpath -u "$APPDATA")/llk2/assets"
```

---

## 6. Run the Verification Suite (`ctest`)

Once assets are extracted into `assets/`:

```bash
ctest --test-dir build --output-on-failure
```

All 8 tests (`test_checklink`, `test_shifts`, `test_scoring_hst`, `test_media`, `test_qt_ui`, `test_imgui_ui`, `test_bui_ui`, `test_launcher_ui`) run headless and should pass (`100% tests passed out of 8`).

---

## 7. Running & Packaging a Standalone `dist/` Folder

### Running from MSYS2 UCRT64

Inside the **MSYS2 UCRT64** shell (where `/ucrt64/bin` is already on `PATH`), you can run any binary directly:

```bash
./build/llk2-launcher.exe
./build/llk2-qt.exe
./build/llk2-imgui.exe
./build/llk2-bui.exe
```

### Creating a Standalone Double-Clickable Folder (`dist/`)

To double-click the `.exe` files directly from **Windows Explorer** (or zip them for another Windows PC without MSYS2 installed), bundle the Qt 6 plugins and required UCRT64 DLLs into `dist/`:

```bash
rm -rf dist && mkdir -p dist
cp build/llk2-*.exe build/soundfont.sf2 dist/
cp -r tools locales dist/

# Deploy Qt 6 DLLs and platform/style/imageformat plugins:
windeployqt6 --no-translations --no-opengl-sw dist/llk2-qt.exe dist/llk2-launcher.exe

# Copy remaining transitive MSYS2 UCRT64 runtime DLLs (SDL3, FreeType, libstdc++, etc.):
for pass in 1 2 3; do
  ldd dist/*.exe dist/*.dll dist/*/*.dll 2>/dev/null \
    | awk '/\/ucrt64\/bin\// {print $3}' \
    | sort -u \
    | xargs -r -I{} cp -u {} dist/
done
```

*(Optionally create an empty `dist/portable.txt` and copy your extracted `assets/` folder into `dist/assets/` to make `dist/` 100% portable on a USB drive.)*

---

## 8. Storage Paths & Legacy `d4s.hst` Migration on Windows

- **Standard Mode**: Stores `config.json`, `scores.json`, `assets/`, `mods/`, and `locales/` in **`%APPDATA%\llk2`** (never writes to `C:\Windows\`).
- **Portable Mode**: Place an empty `portable.txt` next to the `.exe` files (or keep `assets/` / `config.json` in the same writable folder).
- **Migrating Old Scores**: If you played the original 1999 game and still have `C:\Windows\D4s.hst`, open **`llk2-launcher.exe` → High Scores → Import Legacy `d4s.hst`...** (or **Settings → Import Legacy Scores (`d4s.hst`)...** inside any game frontend).
