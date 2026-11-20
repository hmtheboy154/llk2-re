#!/usr/bin/env python3
"""Pure-Python 3.4+ (stdlib only) VB5 asset packer (.frx + Project1.res).

Packs organized assets under <target>/assets/ into VB5 binary files:
  - Project1.res     (32-bit Win32 resource object: CUSTOM 201..206 WAVs, MIDI 101..103)
  - Form_Main.frx    (Icon + 10 PictureBox picture records)
  - Form_Explain.frx (Background picture)
  - Form_Author.frx  (Author credit picture)

Also updates "Form_Main.frx":XXXX hex offsets in <target>/Form_Main.frm when
asset byte sizes change (preserving exact raw bytes and CRLF line endings).

Usage:
  python pack_assets.py src [--prefer-bmp] [--no-icon-frx] [--res-only]
"""
from __future__ import print_function
import os
import re
import struct
import sys

_FORM_MAIN_PICS = (
    ('Pic_Title', 'Pic_Title_Picture'),
    ('Pic_Level_6', 'Pic_Level_6_Picture'),
    ('Pic_Level_5', 'Pic_Level_5_Picture'),
    ('Pic_Level_4', 'Pic_Level_4_Picture'),
    ('Pic_Level_3', 'Pic_Level_3_Picture'),
    ('Pic_Level_2', 'Pic_Level_2_Picture'),
    ('Pic_Level_1', 'Pic_Level_1_Picture'),
    ('Pic_Level_0', 'Pic_Level_0_Picture'),
    ('Pic_Data', 'Pic_Data_Picture'),
    ('Pic_MjData', 'Pic_MjData_Picture'),
)

_EXT_ORDER = ('.bmp', '.jpg', '.jpeg', '.gif')


def _pick_image(images_dir, stem, prefer_bmp=False):
    """Select the image file for <stem> in images_dir.

    If a .bmp copy exists alongside the original .jpg/.gif, picks .bmp when
    prefer_bmp is True OR when the .bmp is newer (e.g. edited in GIMP/assets_mods).
    """
    candidates = {}
    for ext in _EXT_ORDER:
        p = os.path.join(images_dir, stem + ext)
        if os.path.isfile(p):
            candidates[ext] = p
    if not candidates:
        raise FileNotFoundError('Missing asset image %s.{bmp,jpg,gif} in %s' % (stem, images_dir))

    bmp = candidates.get('.bmp')
    orig = None
    for ext in ('.jpg', '.jpeg', '.gif'):
        if ext in candidates:
            orig = candidates[ext]
            break

    if bmp and orig:
        if prefer_bmp or (os.path.getmtime(bmp) > os.path.getmtime(orig) + 0.001):
            return bmp
        return orig
    return bmp or orig


def _pack_frx_records(items):
    """Pack [(key, raw_bytes), ...] into VB5 .frx binary records and return (bytes, {key: offset})."""
    buf = bytearray()
    offsets = {}
    for key, raw in items:
        offsets[key] = len(buf)
        buf += struct.pack('<III', len(raw) + 8, 0x0000746C, len(raw))
        buf += raw
    return bytes(buf), offsets


def _make_res_entry(res_type, res_name, data, lang=0x0404, mem_flags=0x1030):
    """Encode one Win32 32-bit .res resource entry."""
    def _enc(x):
        if isinstance(x, int):
            return struct.pack('<HH', 0xFFFF, x)
        return x.encode('utf-16le') + b'\x00\x00'

    tn = _enc(res_type) + _enc(res_name)
    if len(tn) % 4:
        tn += b'\x00' * (4 - (len(tn) % 4))
    hdr_size = 8 + len(tn) + 16
    hdr = struct.pack('<II', len(data), hdr_size) + tn + struct.pack(
        '<IHHII', 0, mem_flags, lang, 0, 0
    )
    pad = b'\x00' * ((4 - (len(data) % 4)) % 4)
    return hdr + data + pad


def build_res_bytes(sfx_dir, midi_dir, lang=0x0404):
    """Build Win32 Project1.res containing CUSTOM (201..206) and MIDI (101..103)."""
    # Required 32-byte null resource header at start of every 32-bit .res file
    out = bytearray(struct.pack(
        '<IIHHHHIHHII', 0, 32, 0xFFFF, 0, 0xFFFF, 0, 0, 0, 0, 0, 0
    ))

    for wav_id in range(201, 207):
        p = os.path.join(sfx_dir, '%d.wav' % wav_id)
        if not os.path.isfile(p):
            raise FileNotFoundError('Missing SFX asset: %s' % p)
        with open(p, 'rb') as f:
            out += _make_res_entry('CUSTOM', wav_id, f.read(), lang=lang)

    for mid_id in range(101, 104):
        p = os.path.join(midi_dir, '%d.mid' % mid_id)
        if not os.path.isfile(p):
            raise FileNotFoundError('Missing MIDI asset: %s' % p)
        with open(p, 'rb') as f:
            out += _make_res_entry('MIDI', mid_id, f.read(), lang=lang)

    return bytes(out)


def _update_form_main_offsets(frm_path, offsets):
    """Update "Form_Main.frx":XXXX offsets in Form_Main.frm if needed (binary-safe)."""
    with open(frm_path, 'rb') as f:
        raw = f.read()

    lines = raw.split(b'\r\n')
    in_form = False
    cur_ctrl = None
    cur_index = None
    changed = False

    def _slot_key():
        if cur_ctrl is None and in_form:
            return 'Icon'
        if cur_ctrl == 'Pic_Level' and cur_index is not None:
            return 'Pic_Level_%s' % cur_index
        return cur_ctrl

    for i, line in enumerate(lines):
        stripped = line.strip()
        m_form = re.match(rb'^Begin\s+VB\.Form\s+(\w+)\b', stripped)
        if m_form:
            in_form = True
            cur_ctrl = None
            cur_index = None
            continue
        m_ctrl = re.match(rb'^Begin\s+VB\.\w+\s+(\w+)\b', stripped)
        if m_ctrl:
            cur_ctrl = m_ctrl.group(1).decode('ascii', 'replace')
            cur_index = None
            continue
        if stripped == b'End':
            if cur_ctrl is not None:
                cur_ctrl = None
                cur_index = None
            elif in_form:
                in_form = False
            continue
        m_idx = re.match(rb'^Index\s*=\s*(\d+)\b', stripped)
        if m_idx and cur_ctrl is not None:
            cur_index = m_idx.group(1).decode('ascii')
            continue

        m_frx = re.match(
            rb'^(\s*(?:Icon|Picture)\s*=\s*"Form_Main\.frx":)([0-9A-Fa-f]+)(\s*)$',
            line
        )
        if m_frx:
            key = _slot_key()
            if key in offsets:
                new_hex = ('%04X' % offsets[key]).encode('ascii')
                if m_frx.group(2).upper() != new_hex:
                    lines[i] = m_frx.group(1) + new_hex + m_frx.group(3)
                    changed = True

    if changed:
        with open(frm_path, 'wb') as f:
            f.write(b'\r\n'.join(lines))
    return changed


def _frm_has_icon_frx(frm_path):
    with open(frm_path, 'rb') as f:
        raw = f.read()
    return bool(re.search(rb'^\s*Icon\s*=\s*"Form_Main\.frx":', raw, re.MULTILINE))


def pack_project(target_dir, prefer_bmp=False, no_icon_frx=False, res_only=False):
    """Pack <target_dir>/assets into Project1.res and Form_*.frx files."""
    assets_dir = os.path.join(target_dir, 'assets')
    images_dir = os.path.join(assets_dir, 'images')
    icons_dir = os.path.join(assets_dir, 'icons')
    sfx_dir = os.path.join(assets_dir, 'sfx')
    midi_dir = os.path.join(assets_dir, 'midi')

    res_bytes = build_res_bytes(sfx_dir, midi_dir)
    res_path = os.path.join(target_dir, 'Project1.res')
    with open(res_path, 'wb') as f:
        f.write(res_bytes)
    print('[pack] wrote %s (%d bytes)' % (res_path, len(res_bytes)))

    if res_only:
        return

    form_main_frm = os.path.join(target_dir, 'Form_Main.frm')
    include_icon = (not no_icon_frx) and _frm_has_icon_frx(form_main_frm)

    main_items = []
    if include_icon:
        ico_path = os.path.join(icons_dir, 'Form_Main_Icon.ico')
        if not os.path.isfile(ico_path):
            raise FileNotFoundError('Missing icon asset: %s' % ico_path)
        with open(ico_path, 'rb') as f:
            main_items.append(('Icon', f.read()))

    for key, stem in _FORM_MAIN_PICS:
        img_path = _pick_image(images_dir, stem, prefer_bmp=prefer_bmp)
        with open(img_path, 'rb') as f:
            main_items.append((key, f.read()))

    frx_main, offsets = _pack_frx_records(main_items)
    frx_main_path = os.path.join(target_dir, 'Form_Main.frx')
    with open(frx_main_path, 'wb') as f:
        f.write(frx_main)
    updated = _update_form_main_offsets(form_main_frm, offsets)
    print('[pack] wrote %s (%d bytes, icon=%s, frm_updated=%s)' % (
        frx_main_path, len(frx_main), include_icon, updated
    ))

    for frm_name, stem in (
        ('Form_Explain', 'Form_Explain_Picture'),
        ('Form_Author', 'Form_Author_Picture'),
    ):
        img_path = _pick_image(images_dir, stem, prefer_bmp=prefer_bmp)
        with open(img_path, 'rb') as f:
            frx_bytes, _ = _pack_frx_records([('Picture', f.read())])
        out_frx = os.path.join(target_dir, frm_name + '.frx')
        with open(out_frx, 'wb') as f:
            f.write(frx_bytes)
        print('[pack] wrote %s (%d bytes)' % (out_frx, len(frx_bytes)))


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    if not argv or argv[0] in ('-h', '--help'):
        print('Usage: python pack_assets.py <src|src-llkcn> [--prefer-bmp] [--no-icon-frx] [--res-only]')
        return 0 if argv else 1

    target_dir = None
    prefer_bmp = False
    no_icon_frx = False
    res_only = False
    for a in argv:
        if a == '--prefer-bmp':
            prefer_bmp = True
        elif a == '--no-icon-frx':
            no_icon_frx = True
        elif a == '--res-only':
            res_only = True
        elif a.startswith('-'):
            print('Unknown option: %s' % a, file=sys.stderr)
            return 1
        elif target_dir is None:
            target_dir = a
        else:
            print('Unexpected argument: %s' % a, file=sys.stderr)
            return 1

    if not target_dir or not os.path.isdir(target_dir):
        print('Target directory not found: %s' % target_dir, file=sys.stderr)
        return 1

    pack_project(
        target_dir,
        prefer_bmp=prefer_bmp,
        no_icon_frx=no_icon_frx,
        res_only=res_only,
    )
    return 0


if __name__ == '__main__':
    sys.exit(main())
