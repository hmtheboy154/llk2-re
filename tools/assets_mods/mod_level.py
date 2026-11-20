#!/usr/bin/env python3
"""Mod level background images (Pic_Level_0..6_Picture.bmp).

Replaces the decorative backdrop that appears behind the game board.
The game blits Pic_Level(Level) centered into Pic_Buffer at 600×450.

Usage:
  # Replace level 0's background:
  python3 mod_level.py --level 0 --image my_wallpaper.png

  # Replace all levels from a directory:
  #   custom_levels/level0.png, level1.png, ... level6.png
  python3 mod_level.py --bulk custom_levels/

  # Replace ALL 7 levels with the same image:
  python3 mod_level.py --level all --image my_theme.png

  # Preview only (with 16×9 board grid overlay):
  python3 mod_level.py --level 0 --image bg.png --preview
"""
import argparse
import os
import re
import sys
from PIL import Image, ImageDraw

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
DEFAULT_ASSET_DIR = os.path.join(REPO_ROOT, 'src', 'assets', 'images')
DEFAULT_OUTPUT_DIR = os.path.join(SCRIPT_DIR, 'output')
DEFAULT_PREVIEW_DIR = os.path.join(SCRIPT_DIR, 'output', 'previews')

# The game renders Pic_Level images centered into Pic_Buffer (720×550).
LEVEL_W, LEVEL_H = 600, 450
COLS, ROWS = 16, 9
CELL_W, CELL_H = 40, 50
NUM_LEVELS = 7  # levels 0..6

# Board geometry centered within the 600×450 level image (board is 640×450):
BOARD_W = COLS * CELL_W  # 16 × 40 = 640
BOARD_H = ROWS * CELL_H  # 9 × 50 = 450

_LEVEL_PAT = re.compile(r'^Pic_Level_(\d+)_Picture\.(bmp|jpe?g|png|gif)$', re.IGNORECASE)


def find_original_level(asset_dir, level):
    """Return path to original Pic_Level_<level>_Picture.{bmp,jpg,...} if present."""
    if not asset_dir or not os.path.isdir(asset_dir):
        return None
    for ext in ('.bmp', '.jpg', '.jpeg', '.png', '.gif'):
        candidate = os.path.join(asset_dir, f'Pic_Level_{level}_Picture{ext}')
        if os.path.isfile(candidate):
            return candidate
    return None


def detect_level_info(asset_dir):
    """Detect (num_levels, target_w, target_h) from extracted .bmp or .jpg originals."""
    if not asset_dir or not os.path.isdir(asset_dir):
        return NUM_LEVELS, LEVEL_W, LEVEL_H
    found = {}
    for fn in sorted(os.listdir(asset_dir)):
        m = _LEVEL_PAT.match(fn)
        if not m:
            continue
        lvl = int(m.group(1))
        path = os.path.join(asset_dir, fn)
        # Prefer .bmp over .jpg if both exist
        if lvl not in found or fn.lower().endswith('.bmp'):
            found[lvl] = path
    if not found:
        return NUM_LEVELS, LEVEL_W, LEVEL_H
    num_levels = max(NUM_LEVELS, max(found.keys()) + 1)
    first_path = found[min(found.keys())]
    with Image.open(first_path) as im:
        w, h = im.size
    return num_levels, w, h


def load_image(path, target_w=LEVEL_W, target_h=LEVEL_H):
    """Load and resize a custom background image."""
    img = Image.open(path).convert('RGB')
    if img.size != (target_w, target_h):
        img = img.resize((target_w, target_h), Image.LANCZOS)
    return img


def save_bmp(img, path):
    """Save as a 24-bit BMP (for VB5 LoadPicture and pack_assets.py)."""
    img.save(path, format='BMP')


def preview_composite(level_img, path):
    """Render a preview of the level background with 16×9 board grid overlay."""
    w, h = level_img.size
    comp = level_img.copy()
    draw = ImageDraw.Draw(comp)
    bx0 = (w - BOARD_W) // 2
    by0 = (h - BOARD_H) // 2
    for c in range(COLS + 1):
        x = bx0 + c * CELL_W
        draw.line([(x, by0), (x, by0 + BOARD_H)], fill=(255, 255, 0), width=1)
    for r in range(ROWS + 1):
        y = by0 + r * CELL_H
        draw.line([(bx0, y), (bx0 + BOARD_W, y)], fill=(255, 255, 0), width=1)
    comp.save(path)
    print(f'  preview: {path}')


def main():
    parser = argparse.ArgumentParser(
        description='Mod level background images (Pic_Level_0..6_Picture.bmp)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Replace level 0's background:
  %(prog)s --level 0 --image my_wallpaper.png

  # Replace all 7 levels with the same image:
  %(prog)s --level all --image my_theme.png

  # Replace multiple:
  %(prog)s -l 0 -i bg1.png -l 1 -i bg2.png -l 3 -i bg3.png

  # Bulk replace from a directory:
  %(prog)s --bulk custom_levels/
  # (expects level0.png, level1.png, ... level6.png)

Levels map to the game's stages:
  Level 0 = 第一關 (stage 1)
  Level 1 = 第二關
  ...
  Level 6 = 第七關
''')
    parser.add_argument('-l', '--level', type=str, action='append',
                        help='Level number to replace (0-6, or "all")')
    parser.add_argument('-i', '--image', type=str, action='append',
                        help='Custom background image path')
    parser.add_argument('--bulk', type=str,
                        help='Directory of numbered backgrounds (level0.png..level6.png)')
    parser.add_argument('-o', '--output-dir', type=str, default=DEFAULT_OUTPUT_DIR,
                        help='Output directory for modified BMPs '
                             '(default: tools/assets_mods/output)')
    parser.add_argument('--asset-dir', type=str, default=DEFAULT_ASSET_DIR,
                        help='Original extracted images directory '
                             '(default: ../../src/assets/images)')
    parser.add_argument('--preview', action='store_true',
                        help='Generate preview composites with board grid overlay')
    parser.add_argument('--preview-dir', type=str, default=DEFAULT_PREVIEW_DIR,
                        help='Directory for previews')
    args = parser.parse_args()

    num_levels, target_w, target_h = detect_level_info(args.asset_dir)

    replacements = {}
    if args.bulk:
        for fn in sorted(os.listdir(args.bulk)):
            if not fn.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.gif')):
                continue
            base = os.path.splitext(fn)[0].lower().replace('level', '')
            try:
                lvl = int(base)
            except ValueError:
                print(f'  skipping {fn}')
                continue
            if 0 <= lvl < num_levels:
                replacements[lvl] = os.path.join(args.bulk, fn)
    if args.level or args.image:
        if not (args.level and args.image) or len(args.level) != len(args.image):
            print('ERROR: --level and --image counts must match', file=sys.stderr)
            sys.exit(1)
        for lvl, path in zip(args.level, args.image):
            if lvl.lower() == 'all':
                for lv in range(num_levels):
                    replacements[lv] = path
            else:
                try:
                    lv = int(lvl)
                except ValueError:
                    print(f'ERROR: invalid level {lvl!r}', file=sys.stderr)
                    sys.exit(1)
                replacements[lv] = path

    if not replacements:
        if args.preview:
            for lv in range(num_levels):
                orig = find_original_level(args.asset_dir, lv)
                if orig:
                    replacements[lv] = orig
        if not replacements:
            print('No replacements specified. Use --level/--image or --bulk.')
            sys.exit(0)
        preview_only_originals = True
    else:
        preview_only_originals = False

    if not preview_only_originals:
        os.makedirs(args.output_dir, exist_ok=True)
    if args.preview:
        os.makedirs(args.preview_dir, exist_ok=True)

    processed = []
    for lv, path in sorted(replacements.items()):
        if not 0 <= lv < num_levels:
            print(f'  skipping level {lv} (out of range 0-{num_levels - 1})')
            continue
        print(f'  Level {lv}: loading {path}')
        img = load_image(path, target_w=target_w, target_h=target_h)
        if not preview_only_originals:
            out_fn = os.path.join(args.output_dir, f'Pic_Level_{lv}_Picture.bmp')
            save_bmp(img, out_fn)
            print(f'  saved: {out_fn}')
            processed.append((lv, out_fn))
        if args.preview:
            pv = os.path.join(args.preview_dir, f'level_{lv}_preview.png')
            preview_composite(img, pv)

    if processed:
        print('\nTo deploy:')
        for _, src in processed:
            print(f'  External assets: cp {src} build/assets/images/{os.path.basename(src)}')
        print('  Integrated EXE:  bash build_mods.sh --pack  (then rebuild with VB5)')


if __name__ == '__main__':
    main()
