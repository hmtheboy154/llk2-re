# GIMP Asset Modding Guide

How to create custom game assets using GIMP for KAWAI連連看2 (`D4s.exe`).

## Prerequisites
- GIMP 2.10+ installed
- Original asset files extracted into `src/assets/images/` (or `src-llkcn/assets/images/`) via:
  ```bash
  python3 tools/extract_assets.py D4s.exe --bmp
  ```

---

## 1. Modifying Character Sprites (`Pic_MjData_Picture.bmp`)

The character tile sheet is 360×612 pixels and contains:
- **5 rows × 9 columns** of sprite cells, each **40×50** pixels (y=0..250)
- Below them: 5 rows of masks (y=250..500)
- Below them: shell tiles (y=500..612)

### Understanding the Sheet

Each 40×50 cell contains one Pokémon sprite. The game composites
each sprite with a colored shell tile using bitwise operations:

```
final_pixel = (shell_pixel AND mask_pixel) OR art_pixel
```

- **mask = 0 (black)** → the art pixel shows (the sprite's body)
- **mask = 255 (white)** → the shell pixel shows through (transparent)
- **mask = 1-254** → semi-transparent blend (rare, anti-aliased edges)

### Step-by-Step: Replacing One Sprite

1. **Open the sheet in GIMP**:
   - File → Open → `src/assets/images/Pic_MjData_Picture.bmp`

2. **Navigate to the sprite cell you want to modify**:
   - Use Rectangle Select tool (R) with fixed size: 40×50
   - Click on the sprite cell's top-left corner
   - Tile positions: `col × 40`, `row × 50`
   - Example: TileId 5 = col 5, row 0 → select at `(200, 0)`

3. **Create your new character on a separate layer**:
   - Layer → New Layer (name: `"my_character"`)
   - Draw or paste your character image
   - Scale to fit the flat tile face (`32×32` pixels) and place it at offset `(4, 9)` inside the `40×50` cell (`x = 4..35, y = 9..40`) so it does not cover the 3D tile bevel at the outer edges.

4. **Separate the character from the background**:
   - Your character layer should have transparency where the shell shows
   - Use Select → By Color to select the background, then delete it
   - Or use the Fuzzy Select tool (U) to click the background

5. **Generate the art cell** (your character on black):
   - Create a new 40×50 layer filled with BLACK (0,0,0)
   - Copy your character onto it inside the `32×32` flat face box `(4, 9)..(35, 40)` (the character should be OPAQUE, and the surrounding margin stays pure black)
   - This is what appears where mask = 0

6. **Generate the mask cell** (the character's silhouette):
   - Create a new 40×50 layer filled with WHITE (255,255,255)
   - Copy your character's SILHOUETTE (the shape) in BLACK inside the matching `(4, 9)..(35, 40)` box
   - The character body area = BLACK (0,0,0)
   - The surrounding area = WHITE (255,255,255)
   - This is what determines transparency

7. **Paste into the sheet**:
   - Art: copy the character (on black) → Edit → Paste as → New Layer
     → position at the tile's `(col×40, row×50)` → flatten
   - Mask: copy the silhouette → paste at `(col×40, 250 + row×50)` → flatten

8. **Export**:
   - File → Export As → `Pic_MjData_Picture.bmp`
   - Ensure: 24-bit R8G8B8 (not indexed, not 32-bit)

### Tips for Drawing Characters

- The game tiles are small (32×32 flat face inside 40×50 cell) — keep designs simple and bold
- Use high contrast colors — the sprites are viewed at 1:1 scale
- The sprite's "body" should be distinct from its outline
- Dark outlines around the character help it stand out against the shell

### Example: Replacing Bulbasaur (TileId 0) with a Cat

1. Open `src/assets/images/Pic_MjData_Picture.bmp`
2. Select the `40×50` art cell `(0, 0)` to `(39, 49)` and fill with pure black `(0, 0, 0)` to clear Bulbasaur
3. Create a new layer with your cat drawing scaled to fit the `32×32` flat face box (`x = 4..35, y = 9..40`, offset `(4, 9)` inside the cell), keeping the outer bevel margin empty/black
4. Flatten the cat onto the art cell — the cat's colors will show where the mask is black
5. In the corresponding mask cell `(0, 250)` to `(39, 299)`: ensure the cat's silhouette inside `x = 4..35, y = 259..290` is pure black `(0, 0, 0)` and the surrounding area is pure white `(255, 255, 255)`

---

## 2. Modifying Level Backgrounds (`Pic_Level_0..6_Picture.bmp`)

The level backgrounds are 600×450 RGB images that appear behind the
game board as a decorative wallpaper. There are 7 levels (0-6).

### Step-by-Step: Replacing a Level Background

1. **Open the original for reference**:
   - File → Open → `src/assets/images/Pic_Level_0_Picture.bmp`

2. **Create your new background**:
   - File → New → 600×450 pixels
   - Design your background (any art style works)
   - Keep it non-distracting — the game tiles are drawn on top
   - Dark or muted colors work best (the sprites are bright)

3. **Export**:
   - File → Export As → `Pic_Level_0_Picture.bmp`
   - Ensure 24-bit RGB

4. **Deploy** (see Section 6 below)

### Tips for Level Backgrounds

- The game board (16×9 tiles at 40×50) covers most of the center
- Only the edges and gaps between tiles show the background
- Dark backgrounds make the bright sprites stand out
- Avoid detailed patterns — they're mostly hidden behind tiles
- A simple gradient or subtle texture works best

---

## 3. Modifying the Title Screen (`Pic_Title_Picture.bmp`)

The title screen image is 500×375 pixels, displayed centered on the
title screen with the studio banner overlaid.

### Step-by-Step

1. Open `src/assets/images/Pic_Title_Picture.bmp` for reference
2. Create a new 500×375 image
3. Design your title background
4. Export as BMP (24-bit RGB)
5. Deploy (see Section 6 below)

---

## 4. Modifying the HUD Elements (`Pic_Data_Picture.bmp`)

The HUD sheet (510×240) contains:
- Number sprites for score/lives display (`15×25` each at `x = digit * 15, y = 0`)
- Title-screen studio banner (`CHEN PROGRAM STUDY`) at `(220, 0)`, size `290×25`
- Lives icon at `(0, 25)`, size `20×30`
- Timer bar strip at `(20, 25)`, size `300×15`

### Modifying Score Digits

The digits are at fixed positions. Each digit is 15×25 pixels.
The game reads them by column: `digit * 15`.

To create custom digits:
1. Open `src/assets/images/Pic_Data_Picture.bmp` in GIMP
2. The digits 0-9 are at `x = digit*15, y = 0`, each 15×25
3. Replace each digit with your custom design
4. Keep the exact same positions and sizes

### Modifying the Timer Bar & Lives Icon

- **Lives icon**: `(0, 25)`, size `20×30`.
- **Timer bar**: `(20, 25)`, size `300×15`. The game blits a horizontal slice of width `CLng((remSec / TimeLeft) * 300)` from `(20, 25)`.
- **Title studio banner**: `(220, 0)`, size `290×25`.

---

## 5. Quick Reference: File Sizes

| File | Dimensions | Content |
|------|-----------|---------|
| `images/Pic_MjData_Picture.bmp` | 360×612 | Tile sheet (sprites + masks + shells) |
| `images/Pic_Level_0..6_Picture.bmp` | 600×450 | Level backgrounds (7 files) |
| `images/Pic_Title_Picture.bmp` | 500×375 | Title screen background |
| `images/Pic_Data_Picture.bmp` | 510×240 | HUD elements (digits, bars) |
| `images/Form_Author_Picture.bmp` | 370×310 | Author form image |
| `images/Form_Explain_Picture.bmp` | 640×376 | Help/instructions image |
| `icons/Form_Main_Icon.ico` | 32×32 | Application icon |

---

## 6. BMP Export Settings & Deployment

All game image assets must be exported as:
- Format: BMP
- Bit depth: 24-bit R8G8B8 (or 32-bit X8R8G8B8 — the extra channel is ignored)
- Compression: None (`BI_RGB`)
- Color: Full color RGB (not indexed, not grayscale)

In GIMP: **File → Export As → `filename.bmp` → Advanced Options →
Compatibility / Run-Length Encoded unchecked, 24 bits R8G8B8**.

### Deploying Your Edited BMPs

- **Integrated EXE (recommended for distribution):**
  1. Save your edited BMP(s) into `src/assets/images/` (or `src-llkcn/assets/images/`).
  2. Run `python3 tools/pack_assets.py src` to rebuild `Form_Main.frx` (and `Form_Explain.frx` / `Form_Author.frx` / `Project1.res`).
  3. Rebuild `Project1.vbp` with VB5.
- **External Assets (fast hot-swap with `patches/external-assets.patch`):**
  1. Copy your edited BMP(s) to `build/assets/images/` (next to the patched `D4s.exe`, along with `icons/`, `sfx/`, `midi/`).
  2. Restart the game to see the updated assets immediately.
