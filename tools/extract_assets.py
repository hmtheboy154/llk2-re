#!/usr/bin/env python3
"""Pure-Python 3.4+ (stdlib only) asset & resource extractor for KAWAI連連看2.

Unpacks an original packed executable (D4s.exe v1.02 or lianliankan2.ExE
v1.00-cn) completely in memory using unpack_aspack, verifies its SHA-256 hash,
and extracts all images, icons, sound effects, and MIDI music into an organized
assets/{icons,images,sfx,midi} directory tree.

Usage:
  python3 tools/extract_assets.py <D4s.exe|lianliankan2.ExE> [-o|--out <assets_dir>] [--bmp] [--skip-hash]
"""
from __future__ import print_function
import hashlib
import os
import struct
import sys

_TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
if _TOOLS_DIR not in sys.path:
    sys.path.insert(0, _TOOLS_DIR)

import unpack_aspack

KNOWN_HASHES = {
    '72c8a9882badac35faf618e40b751d4c11d1f1cdf935fb94043c41afdba64c7a': 'v1.02',
    'd5306b7fdc3125259d49cee9054eaffebf67237f9240da82b244b932af7c919b': 'v1.00-cn',
}

_FORM_MAIN_NAMES = (
    'Form_Main_Icon',
    'Pic_Title_Picture',
    'Pic_Level_6_Picture',
    'Pic_Level_5_Picture',
    'Pic_Level_4_Picture',
    'Pic_Level_3_Picture',
    'Pic_Level_2_Picture',
    'Pic_Level_1_Picture',
    'Pic_Level_0_Picture',
    'Pic_Data_Picture',
    'Pic_MjData_Picture',
)


def _guess_ext(blob):
    if blob[:4] == b'\x00\x00\x01\x00':
        return '.ico'
    if blob[:2] == b'\xff\xd8':
        return '.jpg'
    if blob[:4] == b'GIF8':
        return '.gif'
    if blob[:2] == b'BM':
        return '.bmp'
    return '.bin'


def _extract_form_media(image, form_name):
    """Extract embedded 0x0000746C picture/icon blobs for a given VB5 form."""
    pat = struct.pack('<H', len(form_name)) + form_name.encode('ascii') + b'\x00\x0d'
    idx = image.find(pat)
    if idx < 9:
        raise ValueError('Form anchor not found: %s' % form_name)
    props_len = struct.unpack_from('<I', image, idx - 9)[0]
    p = idx
    lim = min(len(image), idx - 5 + props_len)
    blobs = []
    while True:
        h = image.find(b'\x6c\x74\x00\x00', p, lim)
        if h < 0 or h < 4 or h + 8 > lim:
            break
        outer = struct.unpack_from('<I', image, h - 4)[0]
        inner = struct.unpack_from('<I', image, h + 4)[0]
        if inner > 16 and outer == inner + 8 and h + 8 + inner <= lim:
            blob = bytes(image[h + 8:h + 8 + inner])
            if _guess_ext(blob) != '.bin':
                blobs.append(blob)
                p = h + 8 + inner
                continue
        p = h + 1
    return blobs


def _parse_rsrc_entries(image, rsrc_rva):
    """Walk the PE .rsrc directory tree and yield (type_key, res_id, data_bytes)."""
    out = []

    def walk(dir_off, depth, path):
        if rsrc_rva + dir_off + 16 > len(image):
            return
        named_cnt, id_cnt = struct.unpack_from('<HH', image, rsrc_rva + dir_off + 12)
        total = named_cnt + id_cnt
        for i in range(total):
            ent = rsrc_rva + dir_off + 16 + i * 8
            if ent + 8 > len(image):
                break
            name_or_id, off_to = struct.unpack_from('<II', image, ent)
            if name_or_id & 0x80000000:
                str_rva = rsrc_rva + (name_or_id & 0x7FFFFFFF)
                slen = struct.unpack_from('<H', image, str_rva)[0]
                key = bytes(image[str_rva + 2:str_rva + 2 + slen * 2]).decode('utf-16le', 'replace')
            else:
                key = name_or_id
            if off_to & 0x80000000:
                walk(off_to & 0x7FFFFFFF, depth + 1, path + (key,))
            else:
                d_rva = rsrc_rva + off_to
                data_rva, data_sz = struct.unpack_from('<II', image, d_rva)
                blob = bytes(image[data_rva:data_rva + data_sz])
                out.append((path + (key,), blob))

    walk(0, 0, ())
    return out


def _win32_to_24bit_bmp(src_path, dst_path):
    """Convert JPG/GIF to 24-bit BI_RGB BMP using native Windows OleLoadPicture + GetDIBits."""
    import ctypes
    from ctypes import wintypes

    kernel32 = ctypes.windll.kernel32
    ole32 = ctypes.windll.ole32
    oleaut32 = ctypes.windll.oleaut32
    gdi32 = ctypes.windll.gdi32
    user32 = ctypes.windll.user32

    kernel32.GlobalAlloc.argtypes = [wintypes.UINT, ctypes.c_size_t]
    kernel32.GlobalAlloc.restype = wintypes.HGLOBAL
    kernel32.GlobalLock.argtypes = [wintypes.HGLOBAL]
    kernel32.GlobalLock.restype = ctypes.c_void_p
    kernel32.GlobalUnlock.argtypes = [wintypes.HGLOBAL]
    kernel32.GlobalUnlock.restype = wintypes.BOOL
    kernel32.GlobalFree.argtypes = [wintypes.HGLOBAL]
    kernel32.GlobalFree.restype = wintypes.HGLOBAL

    ole32.CreateStreamOnHGlobal.argtypes = [
        wintypes.HGLOBAL, wintypes.BOOL, ctypes.POINTER(ctypes.c_void_p)
    ]
    ole32.CreateStreamOnHGlobal.restype = ctypes.c_long

    oleaut32.OleLoadPicture.argtypes = [
        ctypes.c_void_p, ctypes.c_long, wintypes.BOOL,
        ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)
    ]
    oleaut32.OleLoadPicture.restype = ctypes.c_long

    gdi32.GetObjectA.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p]
    gdi32.GetObjectA.restype = ctypes.c_int
    gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
    gdi32.CreateCompatibleDC.restype = wintypes.HDC
    gdi32.DeleteDC.argtypes = [wintypes.HDC]
    gdi32.DeleteDC.restype = wintypes.BOOL
    gdi32.GetDIBits.argtypes = [
        wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT,
        ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT
    ]
    gdi32.GetDIBits.restype = ctypes.c_int
    user32.GetDC.argtypes = [wintypes.HWND]
    user32.GetDC.restype = wintypes.HDC
    user32.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]
    user32.ReleaseDC.restype = ctypes.c_int

    with open(src_path, 'rb') as f:
        data = f.read()

    ole32.CoInitialize(None)
    GMEM_MOVEABLE = 0x0002
    hmem = kernel32.GlobalAlloc(GMEM_MOVEABLE, len(data))
    if not hmem:
        raise RuntimeError('GlobalAlloc failed')
    p_stream = ctypes.c_void_p()
    p_pic = ctypes.c_void_p()
    try:
        ptr = kernel32.GlobalLock(hmem)
        ctypes.memmove(ptr, data, len(data))
        kernel32.GlobalUnlock(hmem)

        hr = ole32.CreateStreamOnHGlobal(hmem, True, ctypes.byref(p_stream))
        if hr != 0:
            raise RuntimeError('CreateStreamOnHGlobal failed: 0x%08X' % (hr & 0xFFFFFFFF))
        hmem = None  # owned by stream now (fDeleteOnRelease=True)

        iid_ipic = ctypes.create_string_buffer(struct.pack(
            '<IHH8s', 0x7BF80980, 0xBF32, 0x101A, b'\x8B\xBB\x00\xAA\x00\x30\x0C\xAB'
        ), 16)
        hr = oleaut32.OleLoadPicture(p_stream, len(data), 0, iid_ipic, ctypes.byref(p_pic))
        if hr != 0 or not p_pic.value:
            raise RuntimeError('OleLoadPicture failed: 0x%08X' % (hr & 0xFFFFFFFF))

        # IUnknown + IPicture vtable call helpers
        vtbl = ctypes.cast(
            ctypes.cast(p_pic, ctypes.POINTER(ctypes.c_void_p))[0],
            ctypes.POINTER(ctypes.c_void_p)
        )
        # Slot 3: get_Handle(void* this, UINT* pHandle)
        get_handle_t = ctypes.WINFUNCTYPE(ctypes.c_long, ctypes.c_void_p, ctypes.POINTER(wintypes.UINT))
        get_handle = get_handle_t(vtbl[3])
        hbm_val = wintypes.UINT()
        hr = get_handle(p_pic, ctypes.byref(hbm_val))
        if hr != 0 or not hbm_val.value:
            raise RuntimeError('IPicture::get_Handle failed: 0x%08X' % (hr & 0xFFFFFFFF))

        bmp_buf = ctypes.create_string_buffer(24)
        if gdi32.GetObjectA(wintypes.HANDLE(hbm_val.value), 24, bmp_buf) == 0:
            raise RuntimeError('GetObjectA failed')
        _, width, height, _, _, _ = struct.unpack_from('<iiiiHH', bmp_buf.raw, 0)
        if width <= 0 or height <= 0:
            raise RuntimeError('Invalid bitmap dimensions: %dx%d' % (width, height))

        stride = ((width * 3 + 3) // 4) * 4
        img_size = stride * height
        bmi_hdr = struct.pack('<IiiHHIIIIII', 40, width, height, 1, 24, 0, img_size, 0, 0, 0, 0)
        bmi_buf = ctypes.create_string_buffer(bmi_hdr, 40)
        px_buf = ctypes.create_string_buffer(img_size)

        hdc_scr = user32.GetDC(None)
        hdc_mem = gdi32.CreateCompatibleDC(hdc_scr)
        user32.ReleaseDC(None, hdc_scr)
        try:
            rows = gdi32.GetDIBits(hdc_mem, wintypes.HBITMAP(hbm_val.value), 0, height, px_buf, bmi_buf, 0)
            if rows != height:
                raise RuntimeError('GetDIBits returned %d rows (expected %d)' % (rows, height))
        finally:
            gdi32.DeleteDC(hdc_mem)

        bfh = struct.pack('<2sIHHI', b'BM', 14 + 40 + img_size, 0, 0, 14 + 40)
        with open(dst_path, 'wb') as f:
            f.write(bfh)
            f.write(bmi_hdr)
            f.write(px_buf.raw)
    finally:
        release_t = ctypes.WINFUNCTYPE(ctypes.c_ulong, ctypes.c_void_p)
        if p_pic.value:
            vtbl = ctypes.cast(ctypes.cast(p_pic, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
            release_t(vtbl[2])(p_pic)
        if p_stream.value:
            vtbl = ctypes.cast(ctypes.cast(p_stream, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
            release_t(vtbl[2])(p_stream)
        if hmem:
            kernel32.GlobalFree(hmem)
        ole32.CoUninitialize()


def _convert_to_bmp(src_path, dst_path):
    if sys.platform == 'win32':
        _win32_to_24bit_bmp(src_path, dst_path)
        return True
    try:
        from PIL import Image
        Image.open(src_path).convert('RGB').save(dst_path, format='BMP')
        return True
    except ImportError:
        return False


def extract_all(exe_path, out_dir=None, make_bmp=False, skip_hash=False):
    """Extract images, icon, SFX WAVs, and MIDIs from <exe_path>."""
    with open(exe_path, 'rb') as f:
        raw = f.read()

    sha256 = hashlib.sha256(raw).hexdigest().lower()
    ver_label = KNOWN_HASHES.get(sha256)
    if ver_label is None:
        msg = 'SHA-256 mismatch for %s: got %s' % (exe_path, sha256)
        if not skip_hash:
            raise ValueError(msg + '\nPass --skip-hash if extracting from a modified binary.')
        print('[extract] WARNING: %s' % msg)
        ver_label = 'custom'
    else:
        print('[extract] Verified %s SHA-256: %s' % (ver_label, sha256))

    if out_dir is None:
        repo_root = os.path.dirname(_TOOLS_DIR)
        assets_dir = os.path.join(repo_root, 'assets')
    else:
        assets_dir = out_dir

    icons_dir = os.path.join(assets_dir, 'icons')
    images_dir = os.path.join(assets_dir, 'images')
    sfx_dir = os.path.join(assets_dir, 'sfx')
    midi_dir = os.path.join(assets_dir, 'midi')
    for d in (icons_dir, images_dir, sfx_dir, midi_dir):
        if not os.path.isdir(d):
            os.makedirs(d)

    image, _, _, info = unpack_aspack.unpack_pe_bytes(raw)

    # 1. Extract form pictures and icon
    main_blobs = _extract_form_media(image, 'Form_Main')
    if len(main_blobs) != len(_FORM_MAIN_NAMES):
        raise RuntimeError('Expected %d Form_Main blobs, got %d' % (len(_FORM_MAIN_NAMES), len(main_blobs)))

    bmp_targets = []
    for slot_name, blob in zip(_FORM_MAIN_NAMES, main_blobs):
        ext = _guess_ext(blob)
        if ext == '.ico':
            p = os.path.join(icons_dir, slot_name + ext)
        else:
            p = os.path.join(images_dir, slot_name + ext)
            bmp_targets.append((p, os.path.join(images_dir, slot_name + '.bmp')))
        with open(p, 'wb') as f:
            f.write(blob)

    for frm_name, out_stem in (('Form_Explain', 'Form_Explain_Picture'), ('Form_Author', 'Form_Author_Picture')):
        blobs = _extract_form_media(image, frm_name)
        if len(blobs) != 1:
            raise RuntimeError('Expected 1 blob in %s, got %d' % (frm_name, len(blobs)))
        blob = blobs[0]
        p = os.path.join(images_dir, out_stem + _guess_ext(blob))
        bmp_targets.append((p, os.path.join(images_dir, out_stem + '.bmp')))
        with open(p, 'wb') as f:
            f.write(blob)

    print('[extract] Extracted 1 icon + 12 images into %s' % assets_dir)

    # 2. Extract PE audio resources (CUSTOM WAVs 201..206 & MIDI 101..103)
    ddir = info['oh'] + 96
    rsrc_rva, _ = struct.unpack_from('<II', image, ddir + 2 * 8)
    rsrc_entries = _parse_rsrc_entries(image, rsrc_rva)
    wav_cnt = 0
    mid_cnt = 0
    for path, blob in rsrc_entries:
        if len(path) < 2:
            continue
        rtype, rid = path[0], path[1]
        if rtype == 'CUSTOM':
            p = os.path.join(sfx_dir, '%s.wav' % rid)
            with open(p, 'wb') as f:
                f.write(blob)
            wav_cnt += 1
        elif rtype == 'MIDI':
            p = os.path.join(midi_dir, '%s.mid' % rid)
            with open(p, 'wb') as f:
                f.write(blob)
            mid_cnt += 1

    print('[extract] Extracted %d SFX WAVs + %d MIDIs' % (wav_cnt, mid_cnt))

    # 3. Optional 24-bit BMP conversion
    if make_bmp:
        converted = 0
        for src_p, dst_p in bmp_targets:
            if not _convert_to_bmp(src_p, dst_p):
                raise RuntimeError(
                    '--bmp requires Pillow on non-Windows systems (pip install pillow)'
                )
            src_st = os.stat(src_p)
            os.utime(dst_p, (src_st.st_atime, src_st.st_mtime))
            converted += 1
        print('[extract] Converted %d images to 24-bit BMP' % converted)


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    if not argv or argv[0] in ('-h', '--help'):
        print(
            'Usage: python3 tools/extract_assets.py <D4s.exe|lianliankan2.ExE> '
            '[-o|--out <assets_dir>] [--bmp] [--skip-hash]'
        )
        return 0 if argv else 1

    exe_path = None
    out_dir = None
    make_bmp = False
    skip_hash = False

    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ('-o', '--out') and i + 1 < len(argv):
            out_dir = argv[i + 1]
            i += 2
        elif a == '--bmp':
            make_bmp = True
            i += 1
        elif a == '--no-pack':
            i += 1
        elif a == '--skip-hash':
            skip_hash = True
            i += 1
        elif a.startswith('-'):
            print('Unknown option: %s' % a, file=sys.stderr)
            return 1
        elif exe_path is None:
            exe_path = a
            i += 1
        else:
            print('Unexpected argument: %s' % a, file=sys.stderr)
            return 1

    if not exe_path or not os.path.isfile(exe_path):
        print('Input executable not found: %s' % exe_path, file=sys.stderr)
        return 1

    extract_all(
        exe_path,
        out_dir=out_dir,
        make_bmp=make_bmp,
        skip_hash=skip_hash,
    )
    return 0


if __name__ == '__main__':
    sys.exit(main())
