# KAWAI連連看2

This project aims to reconstruct a source code that is as close to the game [KAWAI連連看2](https://web.archive.org/web/20060423174908/http://myweb.hinet.net:80/home4/cps-chen/history/d4s/d4s.htm) as possible with the help of AI. Including 2 versions:
- [The official version v1.02](https://web.archive.org/web/20060423174908/http://myweb.hinet.net:80/home4/cps-chen/history/d4s/d4s.htm) made by [CHEN PROGRAM STUDY](https://web.archive.org/web/20070615052321/http://myweb.hinet.net/home4/cps-chen/main.htm).
- [The unofficial Simplified Chinese version v1.00-cn](http://www.llk.cn/LLK2/) made by Casm of [LLK.cn](http://www.llk.cn/) (Previously [www.LoveSnow.com](https://web.archive.org/web/20040729014651/http://www.lovesnow.com/)).

## Project status

The game is fully decompiled, buildable & playable. However, due to the fact that it is being decompiled with LLM, accuracy is still unknown. Any help with evaluation & fixing is appreciated!

Further info from AI:

- **Source reconstruction (`src/` for `v1.02`, `src-llkcn/` for `v1.00-cn`)**: All 57 procedures across 4 forms (`Form_Main`, `Form_Table`, `Form_Explain`, `Form_Author`) and 2 standard modules (`Mainbas`, `BasWave`) are reconstructed directly from the unpacked VB5 native-code binaries without synthetic Wine workarounds or hardcoded asset hacks in the base sources.
- **Native Windows XP SP3 (`zh_TW`) + VB5 Enterprise verification (zero patches)**:
  - `v1.02` integrated build (`D4S.EXE`, 913,408 B): **38/57** procedures match exact byte size (**17/57** byte-normalized identical, **12/57** semantic-signature identical), **0 diffs** across all 4 forms / 53 controls, with exact PE resources (`CUSTOM` 201..206 WAVs, `MIDI` 101..103, `RT_ICON` 30001, `RT_VERSION` `1.2.0.0`).
  - `v1.00-cn` integrated build (`Lianliankan2.exe`, 913,408 B): **37/57** procedures match exact byte size (**17/57** byte-normalized identical, **12/57** semantic-signature identical), **0 diffs** across all 4 forms / 53 controls (raw GBK `Form_Table` captions preserved byte-for-byte), `RT_VERSION` `1.0.0.0`.
- **Wine (Linux) verification**: Verified on Arch Linux with Wine 11.22 (WoW64) across all 4 build modes (integrated ~881 KB and external ~56 KB EXEs for both `v1.02` and `v1.00-cn`) using `patches/wine-compat.patch` and `tools/build_wine.sh`.
- **Optional patches (`patches/`)**:

| Patch | Purpose |
|---|---|
| `patches/wine-compat.patch` | Converts the 5 raw Big5 string lines in `Form_Main.frm` to `ChrW$(&H...)` for Wine VB5 builds |
| `patches/llkcn-fix-midi-error70.patch` | Backports `mciSendStringA "close MyMidi"` to fix MIDI looping Error 70 on `v1.00-cn` |
| `patches/llkcn-fix-margin-crash.patch` | Backports right/bottom board-margin guard in `Pic_Show_MouseDown` onto `v1.00-cn` |
| `patches/external-assets.patch` | Loads `assets/{icons,images,sfx,midi}` from disk at runtime for compact EXEs (~56–94 KB) |

- **Asset modding toolkit (`tools/assets_mods/`)**: Python/PIL scripts (`mod_characters.py`, `mod_level.py`, `build_mods.sh`) and a step-by-step GIMP guide (`tools/assets_mods/GIMP_GUIDE.md`) to customize tile sprites (`Pic_MjData_Picture`) and level backgrounds (`Pic_Level_0..6`), including sample demo PNGs in `custom_sprites/` and `custom_levels/`. See [tools/assets_mods/README.md](tools/assets_mods/README.md).

## Building

### Extract assets from the game

This project requires either the original `D4s.exe` (`v1.02`) or `lianliankan2.ExE` (`v1.00-cn`) to pull the assets out. Check the SHA-256 hash of the files before extracting assets:

| Version | File | SHA-256 |
|---|---|---|
| Official `v1.02` | `D4s.exe` | `72c8a9882badac35faf618e40b751d4c11d1f1cdf935fb94043c41afdba64c7a` |
| Unofficial `v1.00-cn` | `lianliankan2.ExE` | `d5306b7fdc3125259d49cee9054eaffebf67237f9240da82b244b932af7c919b` |

Verify the hash before running extraction:

```cmd
:: Windows 7 / 10 / 11:
certutil -hashfile D4s.exe SHA256
certutil -hashfile lianliankan2.ExE SHA256

:: Windows XP (stock XP lacks certutil; use Python 3.4):
C:\Python34\python.exe -c "import hashlib; print(hashlib.sha256(open('D4s.exe','rb').read()).hexdigest())"
```

```bash
# Linux / macOS:
sha256sum D4s.exe lianliankan2.ExE
```

Then extract the icon, 12 pictures (`.jpg`/`.gif`, plus optional 24-bit `--bmp`), 6 SFX `.wav` files, and 3 `.mid` tracks into `<src|src-llkcn>/assets/` and build `.frx` + `Project1.res`:

```bash
python tools/extract_assets.py D4s.exe --bmp
python tools/extract_assets.py lianliankan2.ExE --bmp
```

### Download Visual Basic 5 Enterprise

Download the **Microsoft Visual Basic 5.0 Enterprise Edition** ISO (`VB50ENT.ISO`) from [WinWorldPC](https://winworldpc.com/product/microsoft-visual-bas/50).

*(Note: extract `VB50ENT.ISO` with `bsdtar`, `xorriso`, or OS virtual-drive mount — `7z`/`7-Zip` fails on this ISO with header errors.)*

### Gather dependencies

- **Python 3.4+** ([`3.4.4`](https://www.python.org/ftp/python/3.4.4/) on Windows XP, [`3.x`](https://www.python.org/downloads/) on modern Windows/Linux) — all core tools (`tools/extract_assets.py`, `tools/pack_assets.py`, `tools/unpack_aspack.py`, `tools/apply_patch.py`) use pure Python standard library only.
- **Pillow (`PIL`)** — optional; needed only on Linux/macOS when passing `--bmp` to `extract_assets.py` or when using `tools/assets_mods/` (`--bmp` on Windows uses built-in Win32 OLE/GDI via `ctypes`).
- **On Linux (optional)**: `wine`, `winetricks`, `git`, `bsdtar`/`xorriso`, plus `fluidsynth`/`qsynth` or `timidity` for MIDI music playback.

### Setup & Build

Follow the platform-specific setup and build instructions:

- **Linux (Wine + VB5 Enterprise)**: [docs/BUILD_WITH_WINE.md](docs/BUILD_WITH_WINE.md)
- **Windows / Windows XP SP3 VM**: [docs/BUILD_WITH_WINDOWS_OR_VM.md](docs/BUILD_WITH_WINDOWS_OR_VM.md)

## Contributing

To audit procedures, compare instructions, or investigate inaccuracies against the original shipped ASPack-packed executables, use the included pure-Python ASPack 2.12 PE unpacker:

```bash
python tools/unpack_aspack.py --pe D4s.exe D4s_unpacked.exe
python tools/unpack_aspack.py --pe lianliankan2.ExE lianliankan2_unpacked.exe
```

This unpacks the compressed `.text`/`.data`/`.rsrc` sections directly from `D4s.exe` or `lianliankan2.ExE` and emits a standalone unpacked PE32 executable (no code execution needed) with reconstructed imports (`MSVBVM50.DLL`) and resources, ready to inspect in Ghidra, IDA, Rizin, or a VB5 decompiler.

Issues, bytecode/disassembly accuracy reports, and pull requests against `src/`, `src-llkcn/`, `patches/`, or `tools/` are very welcome. Keep original copyrighted assets and built binaries out of commits (`.gitignore` excludes `assets/`, `*.frx`, `*.res`, and `*.exe`).

## Credit

- Special thanks to [CHEN PROGRAM STUDY](https://web.archive.org/web/20070615052321/http://myweb.hinet.net/home4/cps-chen/main.htm) for creating the game that is a part of my childhood :).
- [REA (Reverse Engineer Anything)](https://github.com/morluto/rea) MCP server & reverse-engineering toolkit, which powered the binary analysis, PE resource inspection, and decompilation workflow throughout this project.
