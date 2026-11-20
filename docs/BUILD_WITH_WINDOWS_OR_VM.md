# Building KAWAI連連看2 on Windows or a Windows XP VM

This guide covers building **KAWAI連連看2 v1.02** (`src\`) and **v1.00-cn** (`src-llkcn\`) natively with **Microsoft Visual Basic 5.0 Enterprise Edition** on Windows XP (recommended via VM) or modern Windows.

Verified natively inside a **Windows XP Professional SP3 (`zh_TW`)** virtual machine. We verified that a single `zh_TW` Windows XP SP3 environment compiles **both** `v1.02` and `v1.00-cn` with zero form diffs — VB5 preserves `v1.00-cn`'s raw GBK `Form_Table.frm` captions byte-for-byte.

---

## 1. Set Up a Windows XP SP3 (`zh_TW`) VM (Recommended)

Microsoft Visual Basic 5.0 (1997) integrates most reliably on genuine 32-bit **Windows XP Professional SP3 (Traditional Chinese / `zh_TW`)**:

1. Visit the [Massgrave Windows XP links page](https://massgrave.dev/windows_xp_links) and download the **Windows XP Professional SP3 Traditional Chinese (`zh_TW`)** base image together with the corresponding **DVP** patch.
2. Merge the base ISO and DVP patch using **DumbVersion Patcher (DVP)** to obtain a clean retail/VL `zh_TW` SP3 ISO.
3. Install it in **VMware Workstation / Player**, **VirtualBox**, or **QEMU**, then install your guest additions / VMware Tools if desired.

*(On Windows 10/11 directly, VB5 Enterprise can still work via legacy compatibility shims and manually installing 16-bit/32-bit components, though a Windows XP SP3 `zh_TW` VM avoids modern WoW64/UAC/locale quirks.)*

---

## 2. Install Python 3.4.4 & Visual Basic 5.0 Enterprise

### Install Python 3.4.4 (XP-Compatible)

All scripts in `tools\` (`extract_assets.py`, `pack_assets.py`, `unpack_aspack.py`, `apply_patch.py`) use **Python 3.4+ standard library only**:

1. Download [**Python 3.4.4**](https://www.python.org/ftp/python/3.4.4/) (`python-3.4.4.msi`, the final official Python 3 installer for Windows XP) and run it inside the VM.
2. Keep the default path **`C:\Python34\python.exe`**.

### Install Visual Basic 5.0 Enterprise Edition

1. Download `VB50ENT.ISO` from [WinWorldPC](https://winworldpc.com/product/microsoft-visual-bas/50).
2. Mount `VB50ENT.ISO` as your VM's virtual CD/DVD drive (or extract it on the host using `bsdtar`/`xorriso` and copy the folder into the VM — note that **7-Zip fails to extract this ISO** due to big-endian header quirks).
3. Launch **`VB\SETUP\SETUP.EXE`** and step through the wizard:
   - Enter your **Name** (and optional Organization).
   - Enter your **CD Key**.
   - Click **OK** on the **Product ID** confirmation dialog.
   - Keep the default destination **`C:\Program Files\DevStudio\VB`** and click **OK**.
   - Click **I Agree** on the **License Agreement**.
   - Click **Typical** installation (all components required by this project are included).
   - When Setup prompts **`Restart Windows`**, click **Restart Windows** to finish installation.

---

## 3. Extract Original Game Assets

Place the original `D4s.exe` (`v1.02`) and/or `lianliankan2.ExE` (`v1.00-cn`) in the repository root (`C:\llk2-re` in the examples below).

### Verify SHA-256 Hashes First

Stock Windows XP does **not** ship `certutil.exe`, so verify hashes with Python 3.4 on XP (or `certutil -hashfile` on Windows 7+):

```cmd
:: Windows XP (Python 3.4):
C:\Python34\python.exe -c "import hashlib; print(hashlib.sha256(open('D4s.exe','rb').read()).hexdigest())"
C:\Python34\python.exe -c "import hashlib; print(hashlib.sha256(open('lianliankan2.ExE','rb').read()).hexdigest())"

:: Windows 7 / 10 / 11:
certutil -hashfile D4s.exe SHA256
certutil -hashfile lianliankan2.ExE SHA256
```

Expected SHA-256 digests:
- `D4s.exe` (`v1.02`): `72c8a9882badac35faf618e40b751d4c11d1f1cdf935fb94043c41afdba64c7a`
- `lianliankan2.ExE` (`v1.00-cn`): `d5306b7fdc3125259d49cee9054eaffebf67237f9240da82b244b932af7c919b`

### Run `tools\extract_assets.py`

```cmd
cd /d C:\llk2-re
C:\Python34\python.exe tools\extract_assets.py D4s.exe --bmp
C:\Python34\python.exe tools\extract_assets.py lianliankan2.ExE --bmp
```

- Without `--bmp`: extracts `Form_Main_Icon.ico`, original `.jpg`/`.gif` images, SFX WAVs (`201..206.wav`), and MIDIs (`101..103.mid`), and generates `Form_*.frx` + `Project1.res`.
- With `--bmp`: additionally converts the 12 images into 24-bit `BM` `.bmp` files using built-in Win32 OLE/GDI (`OleLoadPicture` + `GetDIBits`) via `ctypes` (zero extra dependencies) while matching original timestamps so unmodified integrated builds keep compact JPG/GIF `.frx` blobs.

---

## 4. Optional Patches (`tools\apply_patch.py`)

Both `src\` (`v1.02`) and `src-llkcn\` (`v1.00-cn`) compile natively on `zh_TW` Windows XP with **zero patches**.

Because stock Windows XP does not include `git` or GNU `patch.exe`, use the included pure-Python binary/CRLF-safe applier `tools\apply_patch.py` (or `git apply` on newer Windows):

- **Backport `v1.02` bug fixes onto `v1.00-cn`** (`Error 70` MCI handle leak + right/bottom tile margin crash):

  ```cmd
  C:\Python34\python.exe tools\apply_patch.py patches\llkcn-fix-midi-error70.patch patches\llkcn-fix-margin-crash.patch
  ```

- **External asset loading (`assets\` folder on disk, compact ~94 KB EXE):**

  ```cmd
  C:\Python34\python.exe tools\apply_patch.py patches\external-assets.patch
  ```

- **Revert patches cleanly:**

  ```cmd
  C:\Python34\python.exe tools\apply_patch.py -R patches\external-assets.patch
  ```

---

## 5. Build the Executables

### Option A: Integrated Standalone Executables (Default)

All images, SFX, MIDIs, and icon are embedded into a standalone ~913 KB EXE (identical resource layout to the originals):

```cmd
cd /d C:\llk2-re
"C:\Program Files\DevStudio\VB\VB5.EXE" /make src\Project1.vbp C:\llk2-re\D4S.EXE /out C:\llk2-re\build_102.log
"C:\Program Files\DevStudio\VB\VB5.EXE" /make src-llkcn\Project1.vbp C:\llk2-re\Lianliankan2.exe /out C:\llk2-re\build_100.log
```

*(You can also double-click `src\Project1.vbp` or `src-llkcn\Project1.vbp` to open it in the VB5 IDE and choose **File -> Make D4S.EXE...**.)*

### Option B: External-Assets Executables

After applying `patches\external-assets.patch`, compile into `src\` and `src-llkcn\` so each ~94 KB EXE sits right alongside its `assets\` directory:

```cmd
cd /d C:\llk2-re
"C:\Program Files\DevStudio\VB\VB5.EXE" /make src\Project1.vbp C:\llk2-re\src\D4S.exe /out C:\llk2-re\build_ext_102.log
"C:\Program Files\DevStudio\VB\VB5.EXE" /make src-llkcn\Project1.vbp C:\llk2-re\src-llkcn\Lianliankan2.exe /out C:\llk2-re\build_ext_100.log
```

---

## 6. Run After Build

- **Integrated build:** copy `D4S.EXE` (or `Lianliankan2.exe`) anywhere and double-click or run it directly — no external `assets\` folder is needed.
- **External-assets build:** keep the compiled EXE in the same folder as `assets\` (`icons\`, `images\`, `sfx\`, `midi\`) and run `src\D4S.exe` (or `src-llkcn\Lianliankan2.exe`).

If you notice `"Windows 無法存取指定的裝置、路徑或檔案..."` when copying an EXE out of Cygwin/tar, run `chmod +x <exe>` in Cygwin or reset NTFS permissions in Windows Explorer properties.
