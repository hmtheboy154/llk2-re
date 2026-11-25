# Building on Linux

This guide covers building **KAWAI連連看2 — Unofficial Remastered** (`llk2-qt`, `llk2-imgui`, `llk2-bui`, and `llk2-launcher`) from source on Linux.

*(Verified on **Arch Linux** with GCC 16, CMake, SDL3, Qt 6, FreeType, Fontconfig, and ALSA.)*

---

## 1. Clone the Repository (with Submodules)

```bash
git clone --recursive -b remastered https://github.com/hmtheboy154/llk2-re.git
cd llk2-re
```

If you already cloned without `--recursive`, initialize submodules with:

```bash
git submodule update --init --recursive
```

---

## 2. Install Build Dependencies

### Arch Linux / Manjaro / EndeavourOS

```bash
sudo pacman -S --needed base-devel cmake ninja pkgconf python \
    sdl3 qt6-base freetype2 fontconfig alsa-lib \
    wayland libxkbcommon libx11 libxcursor libxi libxrandr
```

*(If you prefer SDL2 instead of SDL3, install `sdl2` and pass `-DLLK2_SDL_VERSION=2`.)*

### Debian / Ubuntu (24.04+)

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config python3 \
    libsdl2-dev qt6-base-dev libfreetype-dev libfontconfig-dev libasound2-dev \
    libwayland-dev libxkbcommon-dev libx11-dev libxcursor-dev libxi-dev libxrandr-dev
```

*(On newer releases with SDL3 packaged, you can install `libsdl3-dev` instead of `libsdl2-dev`; CMake automatically prefers SDL3 when found and falls back to SDL2.)*

### Fedora

```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconf-pkg-config python3 \
    SDL3-devel qt6-qtbase-devel freetype-devel fontconfig-devel alsa-lib-devel \
    wayland-devel libxkbcommon-devel libX11-devel libXcursor-devel libXi-devel libXrandr-devel
```

### Recommended CJK & Vietnamese System Fonts

For crisp Traditional Chinese (`zh-TW`), Simplified Chinese (`zh-CN`), and Vietnamese (`vi`) UI rendering across `llk2-qt`, `llk2-imgui`, and `llk2-bui`, install Noto / Source Han fonts if not already present:

```bash
# Arch Linux:
sudo pacman -S --needed noto-fonts noto-fonts-cjk

# Debian / Ubuntu:
sudo apt install -y fonts-noto fonts-noto-cjk

# Fedora:
sudo dnf install -y google-noto-sans-fonts google-noto-sans-cjk-fonts
```

---

## 3. Configure & Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

*(You can omit `-G Ninja` to use standard GNU Make.)*

### CMake Options

| Option | Default | Description |
|---|---|---|
| `LLK2_UI_QT` | `ON` | Build Qt 6 desktop executable (`build/llk2-qt`) |
| `LLK2_UI_IMGUI` | `ON` *(if submodule present)* | Build SDL + Dear ImGui executable (`build/llk2-imgui`) |
| `LLK2_UI_BUI` | `ON` *(if submodule present)* | Build BUI Blender-UI executable (`build/llk2-bui`, isolated `GPL-3.0-or-later`) |
| `LLK2_LAUNCHER` | `ON` | Build standalone Qt 6 launcher & Mod Studio (`build/llk2-launcher`) |
| `LLK2_SDL_VERSION` | `AUTO` | `AUTO` (prefer SDL3, fallback SDL2), `3`, or `2` |
| `LLK2_BUILD_TESTS` | `ON` | Build the 8-target `ctest` verification suite |

For example, to build **only** `CC0-1.0` targets (excluding the optional `GPL-3.0-or-later` `llk2-bui` frontend):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLK2_UI_BUI=OFF
cmake --build build -j"$(nproc)"
```

When the `subprojects/GeneralUser-GS` submodule is initialized, CMake automatically copies `GeneralUser-GS.sf2` next to each built executable as `soundfont.sf2`.

---

## 4. Extract Original Game Assets

See [ASSETS.md](ASSETS.md) for SHA-256 hashes and full details.

- **Via Launcher GUI**: Run `./build/llk2-launcher`, drag and drop `D4s.exe` (or `lianliankan2.ExE` / `.zip`), and click **Extract to Default `assets/`**.
- **Via CLI**:
  ```bash
  # Local portable dev folder:
  python3 tools/extract_assets.py /path/to/D4s.exe -o assets

  # Or unified Linux user data folder:
  python3 tools/extract_assets.py /path/to/D4s.exe -o ~/.local/share/llk2/assets
  ```

---

## 5. Run the Verification Suite (`ctest`)

Once assets are extracted into `assets/` (or `D4s.exe` is in the parent directory):

```bash
ctest --test-dir build --output-on-failure
```

This runs all 8 verification targets (`test_checklink`, `test_shifts`, `test_scoring_hst`, `test_media`, `test_qt_ui`, `test_imgui_ui`, `test_bui_ui`, and `test_launcher_ui`) completely headless.

---

## 6. Running the Game

```bash
# Start the standalone launcher (manage packs, mods, settings & scores):
./build/llk2-launcher

# Or launch any game frontend directly:
./build/llk2-qt
./build/llk2-imgui
./build/llk2-bui
```

### Storage Paths on Linux
- **Installed / Packaged / AppImage**: Uses `$XDG_DATA_HOME/llk2` (defaults to **`~/.local/share/llk2`**) for `config.json`, `scores.json`, `assets/`, `mods/`, and `locales/`.
- **Local Dev / Portable Mode**: Running inside a writable checkout containing `assets/` or `config.json` (or placing an empty `portable.txt` next to the binaries) keeps all runtime files in that folder automatically.
- **Explicit Overrides**: Pass `--data-dir <dir>` and/or `--assets <dir>`, or set `LLK2_DATA_DIR=<dir>`.
