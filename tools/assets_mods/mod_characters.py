#!/usr/bin/env python3
"""Mod the character sprites in Pic_MjData_Picture.{bmp,jpg}.

Replaces individual Pokémon sprites in the game's tile sheet with custom
images. Handles the sheet's SRCAND/SRCPAINT compositing format:

  Sheet layout (360×612):
    y   0-250:  art cells    (5 rows × 9 cols, each 40×50) — colored sprite on black
    y 250-500:  mask cells   (5 rows × 9 cols, each 40×50) — silhouette mask
    y 500-612:  shell tiles  (6 palette variants + shell mask)

  TileId → cell position:
    row = TileId // 9,  col = TileId % 9
    art cell:  (col*40,     row*50)
    mask cell: (col*40, 250 + row*50)

Usage:
  # Replace a single character (TileId 0 = first sprite):
  python3 mod_characters.py --sprite 0 --image my_char.png

  # Replace multiple characters:
  python3 mod_characters.py --sprite 0 --image cat.png \
                            --sprite 1 --image dog.png \
                            --sprite 5 --image bird.png

  # Replace ALL characters from a directory of numbered sprites:
  #   custom_sprites/00.png, 01.png, ... 41.png
  python3 mod_characters.py --bulk custom_sprites/

  # Preview only (don't save):
  python3 mod_characters.py --sprite 0 --image my_char.png --preview

  # Output to a specific path:
  python3 mod_characters.py --sprite 0 --image my_char.png -o output/Pic_MjData_Picture.bmp
"""
import argparse
import os
import sys
import numpy as np
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
DEFAULT_IMAGES_DIR = os.path.join(REPO_ROOT, 'src', 'assets', 'images')
DEFAULT_OUTPUT = os.path.join(SCRIPT_DIR, 'output', 'Pic_MjData_Picture.bmp')
DEFAULT_PREVIEW_DIR = os.path.join(SCRIPT_DIR, 'output', 'previews')

# --- sheet geometry constants ---
SHEET_W, SHEET_H = 360, 612
CELL_W, CELL_H = 40, 50
ART_Y0 = 0          # art cells start at y=0
MASK_Y0 = 250       # mask cells start at y=250
SHELL_Y0 = 500      # shell tiles start at y=500
NUM_ROWS = 5
NUM_COLS = 9
NUM_TILES = 42      # TileIds 0..41 used by InitBoard


def resolve_default_sheet(images_dir=DEFAULT_IMAGES_DIR):
    """Resolve Pic_MjData_Picture.bmp, falling back to .jpg if .bmp doesn't exist."""
    bmp_path = os.path.join(images_dir, 'Pic_MjData_Picture.bmp')
    if os.path.isfile(bmp_path):
        return bmp_path
    jpg_path = os.path.join(images_dir, 'Pic_MjData_Picture.jpg')
    if os.path.isfile(jpg_path):
        return jpg_path
    return bmp_path


def tile_pos(tile_id):
    """Return (col, row, art_x, art_y, mask_x, mask_y) for a TileId."""
    row = tile_id // NUM_COLS
    col = tile_id % NUM_COLS
    art_x = col * CELL_W
    art_y = ART_Y0 + row * CELL_H
    mask_x = col * CELL_W
    mask_y = MASK_Y0 + row * CELL_H
    return col, row, art_x, art_y, mask_x, mask_y


def load_sheet(path):
    """Load the original sheet (.bmp or .jpg) as a mutable numpy array (H, W, 3)."""
    img = Image.open(path).convert('RGB')
    if img.size != (SHEET_W, SHEET_H):
        print(f'WARNING: sheet size {img.size} != ({SHEET_W},{SHEET_H}), resizing')
        img = img.resize((SHEET_W, SHEET_H), Image.LANCZOS)
    return np.asarray(img, dtype=np.uint8).copy()


def save_sheet(arr, path):
    """Save the modified sprite sheet as a 24-bit uncompressed RGB BMP."""
    Image.fromarray(arr).save(path, format='BMP')


def load_sprite_image(path, target_w=CELL_W, target_h=CELL_H,
                      fit_w=32, fit_h=32, box_x=4, box_y=9):
    """Load a custom sprite image and prepare it for the sheet.

    Returns (art_cell, mask_cell) as (H, W, 3) uint8 arrays.
    Art cell: the sprite on a black 40×50 background.
    Mask cell: black sprite silhouette on a white 40×50 background.
    Original sprites occupy the 32×32 flat face region x=4..35, y=9..40
    so they do not overwrite the 3D shell bevel at the cell edges.
    """
    img = Image.open(path)

    # Handle alpha channel
    has_alpha = img.mode in ('RGBA', 'LA') or (
        img.mode == 'P' and 'transparency' in img.info)

    if has_alpha:
        img_rgba = img.convert('RGBA')
        alpha = np.asarray(img_rgba, dtype=np.uint8)[:, :, 3]
        rgb = np.asarray(img_rgba, dtype=np.uint8)[:, :, :3]
    else:
        # No alpha — use the corner pixels as the background color key
        img_rgb = img.convert('RGB')
        rgb = np.asarray(img_rgb, dtype=np.uint8)
        corners = [rgb[0, 0], rgb[0, -1], rgb[-1, 0], rgb[-1, -1]]
        bg_color = np.median(np.array(corners, dtype=np.float32), axis=0).astype(np.uint8)
        dist = np.abs(rgb.astype(np.int16) - bg_color.astype(np.int16)).max(axis=2)
        alpha = np.where(dist > 30, 255, 0).astype(np.uint8)

    # Scale to fit within the 32×32 interior face area while maintaining aspect ratio
    h, w = rgb.shape[0], rgb.shape[1]
    scale = min(fit_w / w, fit_h / h)
    new_w = max(1, int(round(w * scale)))
    new_h = max(1, int(round(h * scale)))

    pil_alpha = Image.fromarray(alpha).resize((new_w, new_h), Image.LANCZOS)
    pil_rgb = Image.fromarray(rgb).resize((new_w, new_h), Image.LANCZOS)
    alpha = np.asarray(pil_alpha, dtype=np.uint8)
    rgb = np.asarray(pil_rgb, dtype=np.uint8)

    # Center inside the 32×32 flat face box (x=4..35, y=9..40)
    ox = box_x + (fit_w - new_w) // 2
    oy = box_y + (fit_h - new_h) // 2

    # ART cell: sprite on black background
    art_cell = np.zeros((target_h, target_w, 3), dtype=np.uint8)
    spr_mask = np.zeros((target_h, target_w), dtype=bool)
    spr_mask[oy:oy + new_h, ox:ox + new_w] = alpha > 128
    art_cell[spr_mask] = rgb[alpha > 128]

    # MASK cell: black (0) where the sprite is, white (255) elsewhere
    mask_cell = np.full((target_h, target_w, 3), 255, dtype=np.uint8)
    mask_cell[spr_mask] = 0

    return art_cell, mask_cell


def make_mask_from_art(art_cell):
    """Generate the mask cell from the art cell: non-black → mask 0 (sprite area)."""
    non_black = art_cell.max(axis=2) > 10
    mask_cell = np.full(art_cell.shape[:2] + (3,), 255, dtype=np.uint8)
    mask_cell[non_black] = 0
    return mask_cell


def apply_sprite(sheet, tile_id, art_cell, mask_cell):
    """Paste an art cell and mask cell into the sheet at the TileId position."""
    col, row, ax, ay, mx, my = tile_pos(tile_id)
    sheet[ay:ay + CELL_H, ax:ax + CELL_W] = art_cell
    sheet[my:my + CELL_H, mx:mx + CELL_W] = mask_cell


def preview_tile(sheet, tile_id, output_path):
    """Render a preview of a tile as it would appear in-game."""
    col, row, ax, ay, mx, my = tile_pos(tile_id)
    art_cell = sheet[ay:ay + CELL_H, ax:ax + CELL_W].copy()
    mask_cell = sheet[my:my + CELL_H, mx:mx + CELL_W].copy()
    shell = sheet[SHELL_Y0:SHELL_Y0 + CELL_H, 0:CELL_W].copy()
    dest = (shell & mask_cell) | art_cell
    Image.fromarray(dest).resize((CELL_W * 4, CELL_H * 4), Image.NEAREST).save(output_path)


def main():
    default_sheet = resolve_default_sheet()
    parser = argparse.ArgumentParser(
        description='Mod character sprites in Pic_MjData_Picture.{bmp,jpg}',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Replace TileId 0 with a custom sprite:
  %(prog)s --sprite 0 --image my_cat.png

  # Replace multiple:
  %(prog)s -s 0 -i cat.png -s 1 -i dog.png -s 5 -i bird.png

  # Bulk replace from a directory (00.png..41.png):
  %(prog)s --bulk custom_sprites/

  # Preview a tile:
  %(prog)s --sprite 0 --image my_cat.png --preview

Tile IDs map to sheet positions:
  TileId 0-8:   row 0 (top)
  TileId 9-17:  row 1
  TileId 18-26: row 2
  TileId 27-35: row 3
  TileId 36-41: row 4 (bottom, cols 0-5)

Each class has 4 tiles with the same TileId. The game's matching uses
MatchId which equals TileId for regular tiles (0-33) and groups 4 flower
sprites for TileIds 34-37 (MatchId 34) and 38-41 (MatchId 35).
''')
    parser.add_argument('-s', '--sprite', type=int, action='append',
                        help='TileId to replace (0-41)')
    parser.add_argument('-i', '--image', type=str, action='append',
                        help='Custom sprite image path (PNG/JPG/BMP)')
    parser.add_argument('--bulk', type=str,
                        help='Directory of numbered sprites (00.png..41.png)')
    parser.add_argument('-o', '--output', type=str, default=DEFAULT_OUTPUT,
                        help='Output path for the modified BMP sheet '
                             '(default: tools/assets_mods/output/Pic_MjData_Picture.bmp)')
    parser.add_argument('--sheet', type=str, default=None,
                        help='Original sheet to modify '
                             '(default: ../../src/assets/images/Pic_MjData_Picture.bmp, '
                             'falling back to .jpg)')
    parser.add_argument('--preview', action='store_true',
                        help='Generate preview images without saving the modified sheet')
    parser.add_argument('--preview-dir', type=str, default=DEFAULT_PREVIEW_DIR,
                        help='Directory for preview images')
    args = parser.parse_args()

    sheet_path = args.sheet if args.sheet is not None else default_sheet
    if not os.path.isfile(sheet_path) and args.sheet is not None and sheet_path.lower().endswith('.bmp'):
        alt_jpg = sheet_path[:-4] + '.jpg'
        if os.path.isfile(alt_jpg):
            sheet_path = alt_jpg
    if not os.path.isfile(sheet_path):
        print(f'ERROR: original sprite sheet not found: {sheet_path}', file=sys.stderr)
        print('Extract original assets first (e.g. python3 tools/extract_assets.py D4s.exe --bmp)',
              file=sys.stderr)
        sys.exit(1)

    sheet = load_sheet(sheet_path)
    print(f'Loaded sheet {sheet_path} ({SHEET_W}×{SHEET_H})')

    replacements = []
    if args.sprite or args.image:
        if not (args.sprite and args.image) or len(args.sprite) != len(args.image):
            print('ERROR: --sprite and --image counts must match', file=sys.stderr)
            sys.exit(1)
        for tid, path in zip(args.sprite, args.image):
            replacements.append((tid, path))
    if args.bulk:
        for fn in sorted(os.listdir(args.bulk)):
            if not fn.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.gif')):
                continue
            base = os.path.splitext(fn)[0]
            try:
                tid = int(base)
            except ValueError:
                print(f'  skipping {fn} (filename not a number)')
                continue
            if 0 <= tid < NUM_TILES:
                replacements.append((tid, os.path.join(args.bulk, fn)))

    if not replacements:
        print('No replacements specified. Use --sprite/--image or --bulk.')
        sys.exit(0)

    previews = []
    for tid, path in replacements:
        if not 0 <= tid < NUM_TILES:
            print(f'  skipping TileId {tid} (out of range 0-{NUM_TILES - 1})')
            continue
        print(f'  TileId {tid}: loading {path}')
        art_cell, mask_cell = load_sprite_image(path)
        apply_sprite(sheet, tid, art_cell, mask_cell)
        previews.append((tid, path))

    if args.preview or previews:
        os.makedirs(args.preview_dir, exist_ok=True)
        for tid, _ in previews:
            pv_path = os.path.join(args.preview_dir, f'tile_{tid:02d}_preview.png')
            preview_tile(sheet, tid, pv_path)
            print(f'  preview: {pv_path}')

    if not args.preview:
        os.makedirs(os.path.dirname(args.output) or '.', exist_ok=True)
        save_sheet(sheet, args.output)
        print(f'\nSaved modified sheet: {args.output}')
        print(f'  {len(previews)} sprites replaced')
        print('\nTo deploy:')
        print(f'  External assets: cp {args.output} build/assets/images/Pic_MjData_Picture.bmp')
        print('  Integrated EXE:  bash build_mods.sh --pack  (then rebuild with VB5)')
    else:
        print('Preview mode — sheet not saved')


if __name__ == '__main__':
    main()
