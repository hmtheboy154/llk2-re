# KAWAI連連看2 — Asset Modding Toolkit

Tools and documentation for modding custom graphics and audio into the rebuilt
**KAWAI連連看2** (`src/` for official `v1.02`, or `src-llkcn/` for `v1.00-cn`).
Two visual editing methods are available:

1. **Script-based** (Python + PIL) — automated, batch-friendly
2. **GIMP-based** (manual) — visual, artist-friendly

## Directory Structure

```
tools/assets_mods/
├── README.md              ← this file
├── mod_characters.py      ← replace tile sprites (Pic_MjData_Picture)
├── mod_level.py           ← replace level backgrounds (Pic_Level_*)
├── GIMP_GUIDE.md          ← step-by-step GIMP instructions
├── build_mods.sh          ← one-command mod + pack/deploy pipeline
├── custom_sprites/        ← PUT YOUR CHARACTER IMAGES HERE
│   ├── 00.png             ← sprite for TileId 0
│   ├── 01.png             ← sprite for TileId 1
│   ├── ...
│   └── 41.png             ← sprite for TileId 41 (flower)
├── custom_levels/         ← PUT YOUR BACKGROUND IMAGES HERE
│   ├── level0.png         ← background for Level 0
│   ├── level1.png         ← background for Level 1
│   ├── ...
│   └── level6.png         ← background for Level 6
└── output/                ← generated BMPs & previews go here (gitignored)
```

Before running any modding scripts, extract the original assets from your
original game executable using `tools/extract_assets.py` (pass `--bmp` if you
want ready-to-edit 24-bit BMPs as well as `.jpg`/`.gif` originals):

```bash
# Example for official v1.02 (from the repository root):
python3 tools/extract_assets.py D4s.exe --bmp
# Or for v1.00-cn:
python3 tools/extract_assets.py lianliankan2.ExE --bmp
```

This populates `src/assets/` (or `src-llkcn/assets/`) with `icons/`, `images/`,
`sfx/`, and `midi/`.

## Quick Start (Script Method)

Run these commands from `tools/assets_mods/` (they also work from the repository
root as `python3 tools/assets_mods/...`):

### 1. Replace Level Backgrounds

```bash
# One level:
python3 mod_level.py --level 0 --image my_wallpaper.png

# All 7 levels with the same image:
python3 mod_level.py --level all --image my_theme.png

# Or put your images in custom_levels/ as level0.png..level6.png:
python3 mod_level.py --bulk custom_levels/
```

### 2. Replace Character Sprites

```bash
# Replace one character (TileId 0):
python3 mod_characters.py --sprite 0 --image my_cat.png

# Replace several:
python3 mod_characters.py -s 0 -i cat.png -s 1 -i dog.png -s 5 -i bird.png

# Or bulk-replace from a directory (00.png..41.png):
python3 mod_characters.py --bulk custom_sprites/
```

`mod_characters.py` defaults to `../../src/assets/images/Pic_MjData_Picture.bmp`
and automatically falls back to `Pic_MjData_Picture.jpg` if `.bmp` has not been
generated. Use `--sheet ../../src-llkcn/assets/images/Pic_MjData_Picture.bmp` to
target `src-llkcn/`.

### 3. Build and Deploy

Two workflows are supported depending on how you build the executable:

#### Workflow A: Integrated Rebuild (`--pack` + VB5 Build)

Embeds modded BMPs directly into `Form_Main.frx` (and `Project1.res`) so the
compiled `.exe` remains completely self-contained:

```bash
# Generate modded BMPs, copy them into src/assets/images/, and re-pack .frx/.res:
bash build_mods.sh --pack

# Or target v1.00-cn:
bash build_mods.sh --version src-llkcn --pack

# Then compile with VB5 (e.g. bash tools/build.sh src/Project1.vbp build/D4s.exe)
```

> **Note:** `--pack` copies your modded BMPs into `src/assets/images/` before
> running `tools/pack_assets.py`. Re-run `python3 tools/extract_assets.py D4s.exe --bmp`
> whenever you want to revert to unmodded embedded assets.

#### Workflow B: External-Assets Hot-Swap (`external-assets.patch` + `build_mods.sh`)

When your executable is built with `patches/external-assets.patch`, images and
audio are loaded from `assets/` next to the `.exe` so you can swap mods without
recompiling:

```bash
# Generate modded BMPs and deploy them to build/assets/images/
# (plus copy unmodded icons/, images/, sfx/, midi/ from src/assets/):
bash build_mods.sh

# Or target src-llkcn / a custom GAME_DIR:
GAME_DIR=../../build bash build_mods.sh --version src-llkcn

# Restore unmodded original assets into build/assets/:
bash build_mods.sh --restore
```

Restart the game after deploying to see your changes.

## Modifying Sound Effects & Background Music

Extracted audio files live alongside images under `<target>/assets/`:

- **Sound effects** (`assets/sfx/201.wav`..`206.wav`): standard uncompressed PCM
  `.wav` files played via `sndPlaySoundA`.
- **Background music** (`assets/midi/101.mid`..`103.mid`): Standard MIDI files
  (`.mid`). Note that official `v1.02` rotates tracks `101..102`, while
  `v1.00-cn` rotates `101..103`.

To replace any sound effect or track:

1. Place your replacement `.wav` or `.mid` under `src/assets/sfx/` or
   `src/assets/midi/` (or `src-llkcn/assets/...`) using the exact numeric
   filename (`201.wav`..`206.wav`, `101.mid`..`103.mid`).
2. Apply your change using either workflow:
   - **Integrated EXE**: re-pack resources (`python3 tools/pack_assets.py src`)
     and rebuild the project in VB5.
   - **External assets** (`patches/external-assets.patch`): copy the updated
     `.wav` / `.mid` files into `build/assets/sfx/` or `build/assets/midi/`
     (or run `bash tools/assets_mods/build_mods.sh --restore` to re-sync all
     subdirectories from `src/assets/`).

## How the Game Renders Tiles

Each game tile is composited from three layers in `Pic_MjData_Picture`:

```
1. Shell tile (colored 3D tile background) — from the bottom of the sheet
2. Face mask (sprite silhouette) — black = sprite area, white = transparent
3. Face art (sprite colors) — colored pixels on black background
```

The game composites them:
```
result = (shell AND mask) OR art
```

- Where mask = 0 (black): the art's color shows (the character's body)
- Where mask = 255 (white): the shell shows through (transparent)
- The mask defines the character's silhouette (which pixels are "body")

## Tile ID Reference

The sheet has 45 slots (5 rows × 9 cols), 42 used by `InitBoard`:

| TileId | Sheet Position | Class | Notes |
|--------|---------------|-------|-------|
| 0-33 | row `t // 9`, col `t % 9` | `MatchId = TileId` | Regular Pokémon (4 copies each; rows 0–1 Gen-1, rows 2–3 Gen-2) |
| 34 | row 3, col 7 | `MatchId 34` | Flower group 1 (Politoed in original sheet) |
| 35 | row 3, col 8 | `MatchId 34` | Flower group 1 (Politoed in original sheet) |
| 36 | row 4, col 0 | `MatchId 34` | Flower group 1 (Politoed in original sheet) |
| 37 | row 4, col 1 | `MatchId 34` | Flower group 1 (Politoed in original sheet) |
| 38 | row 4, col 2 | `MatchId 35` | Flower group 2 (Omanyte in original sheet) |
| 39 | row 4, col 3 | `MatchId 35` | Flower group 2 (Omanyte in original sheet) |
| 40 | row 4, col 4 | `MatchId 35` | Flower group 2 (Omanyte in original sheet) |
| 41 | row 4, col 5 | `MatchId 35` | Flower group 2 (Omanyte in original sheet) |

**Note on Flower Tiles (`TileId 34..41`)**: In `InitBoard`, `t = 136..139` get
`TileId = 34..37` and `MatchId = 34`, while `t = 140..143` get `TileId = 38..41`
and `MatchId = 35`. In the original sheet, `34..37` are 4 identical Politoed
sprites and `38..41` are 4 identical Omanyte sprites, but because they have
distinct `TileId` slots on the sheet, modders can optionally place 4 different
sprites into `34..37` (and 4 different sprites into `38..41`) and any pair
within the same `MatchId` group will still match.

## Sprite Format Requirements

Each sprite cell in the sheet:
- Cell size: `40×50` pixels, with the flat tile face occupying `x = 4..35, y = 9..40` (`32×32` pixels) so the 3D shell bevel at `x = 0..3, y = 0..8` and bottom/right border remain visible.
- Background: BLACK (0,0,0) — the transparent/unused area
- Sprite: colored pixels (any color except pure black)
- Position: centered inside the `32×32` flat face box (`x = 4..35, y = 9..40`)

The mask cell (below the art cell):
- Size: 40×50 pixels
- Background: WHITE (255,255,255) — the transparent area
- Sprite silhouette: BLACK (0,0,0) — the opaque area
- The silhouette should match the sprite's shape exactly

## Method 2: GIMP Manual Editing

See `GIMP_GUIDE.md` for step-by-step instructions on using GIMP
to create custom sprites and level backgrounds manually.
