#!/usr/bin/env bash
# build_mods.sh — one-command asset mod pipeline
# Applies custom sprites/backgrounds and either packs them for an integrated
# VB5 rebuild (--pack) or deploys external assets into $GAME_DIR/assets/.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TARGET_SRC="${TARGET_SRC:-src}"
GAME_DIR="${GAME_DIR:-$PROJROOT/build}"
PYTHON="${PYTHON:-python3}"
PACK=0
RESTORE=0

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --version <src|src-llkcn>  Target source tree (default: \$TARGET_SRC or src)
  --pack                     Copy modded BMPs into <target>/assets/images/ and
                             run tools/pack_assets.py to embed into .frx/.res
  --restore                  Restore unmodded assets from <target>/assets/ into
                             \$GAME_DIR/assets/ (or re-pack with --pack)
  -h, --help                 Show this help message

Environment:
  TARGET_SRC   Target source folder: src (default) or src-llkcn
  GAME_DIR     External-assets deploy directory (default: \$PROJROOT/build)
  PYTHON       Python 3 interpreter (default: python3)
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --version)
            [ $# -ge 2 ] || { echo "ERROR: --version requires an argument" >&2; exit 1; }
            TARGET_SRC="$2"
            shift 2
            ;;
        --version=*)
            TARGET_SRC="${1#--version=}"
            shift
            ;;
        --pack)
            PACK=1
            shift
            ;;
        --restore)
            RESTORE=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "ERROR: unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

case "$TARGET_SRC" in
    src|src-llkcn) ;;
    *)
        echo "ERROR: --version / TARGET_SRC must be 'src' or 'src-llkcn' (got '$TARGET_SRC')" >&2
        exit 1
        ;;
esac

SRC_ASSETS="$PROJROOT/$TARGET_SRC/assets"

echo "=== KAWAI連連看2 Asset Mod Pipeline ==="
echo "  project:    $PROJROOT"
echo "  target src: $TARGET_SRC"
if [ "$PACK" -eq 1 ]; then
    echo "  mode:       pack (embed into $TARGET_SRC)"
else
    echo "  mode:       external assets ($GAME_DIR/assets)"
fi

if [ "$RESTORE" -eq 1 ]; then
    if [ ! -d "$SRC_ASSETS" ]; then
        echo "ERROR: $SRC_ASSETS not found. Extract original assets first:" >&2
        echo "  $PYTHON $PROJROOT/tools/extract_assets.py <D4s.exe|lianliankan2.ExE> --out $TARGET_SRC --bmp" >&2
        exit 1
    fi
    if [ "$PACK" -eq 1 ]; then
        echo "Re-packing assets from $SRC_ASSETS ..."
        "$PYTHON" "$PROJROOT/tools/pack_assets.py" "$PROJROOT/$TARGET_SRC"
        echo "Note: if you previously ran --pack with modded BMPs, re-run extract_assets.py --bmp first to restore unmodded originals in $SRC_ASSETS/images/."
        exit 0
    fi
    for subdir in icons images sfx midi; do
        if [ -d "$SRC_ASSETS/$subdir" ]; then
            rm -rf "$GAME_DIR/assets/$subdir"
            mkdir -p "$GAME_DIR/assets/$subdir"
            cp -a "$SRC_ASSETS/$subdir/." "$GAME_DIR/assets/$subdir/"
        fi
    done
    echo "Restored original unmodded assets from $SRC_ASSETS/ into $GAME_DIR/assets/"
    exit 0
fi

# 1. Apply character sprite mods
echo ""
echo "--- [1/3] Character sprites ---"
if ls "$SCRIPT_DIR/custom_sprites/"*.png "$SCRIPT_DIR/custom_sprites/"*.jpg "$SCRIPT_DIR/custom_sprites/"*.jpeg "$SCRIPT_DIR/custom_sprites/"*.bmp 2>/dev/null | head -1 > /dev/null 2>&1; then
    ORIG_SHEET="$SRC_ASSETS/images/Pic_MjData_Picture.bmp"
    if [ ! -f "$ORIG_SHEET" ]; then
        ORIG_SHEET="$SRC_ASSETS/images/Pic_MjData_Picture.jpg"
    fi
    if [ ! -f "$ORIG_SHEET" ]; then
        echo "ERROR: original sprite sheet not found in $SRC_ASSETS/images/" >&2
        echo "Run tools/extract_assets.py first." >&2
        exit 1
    fi
    cd "$PROJROOT"
    "$PYTHON" "$SCRIPT_DIR/mod_characters.py" \
        --bulk "$SCRIPT_DIR/custom_sprites/" \
        --sheet "$ORIG_SHEET" \
        --output "$SCRIPT_DIR/output/Pic_MjData_Picture.bmp"
else
    echo "  no custom sprites found, skipping"
fi

# 2. Apply level background mods
echo ""
echo "--- [2/3] Level backgrounds ---"
if ls "$SCRIPT_DIR/custom_levels/"*.png "$SCRIPT_DIR/custom_levels/"*.jpg "$SCRIPT_DIR/custom_levels/"*.jpeg "$SCRIPT_DIR/custom_levels/"*.bmp 2>/dev/null | head -1 > /dev/null 2>&1; then
    cd "$PROJROOT"
    "$PYTHON" "$SCRIPT_DIR/mod_level.py" \
        --bulk "$SCRIPT_DIR/custom_levels/" \
        --asset-dir "$SRC_ASSETS/images" \
        --output-dir "$SCRIPT_DIR/output/"
else
    echo "  no custom levels found, skipping"
fi

# 3. Pack or deploy
echo ""
if [ "$PACK" -eq 1 ]; then
    echo "--- [3/3] Packing modded BMPs into $TARGET_SRC ---"
    mkdir -p "$SRC_ASSETS/images"
    packed=0
    for f in "$SCRIPT_DIR/output/"*.bmp; do
        if [ -f "$f" ]; then
            base=$(basename "$f")
            cp "$f" "$SRC_ASSETS/images/$base"
            echo "  $TARGET_SRC/assets/images/$base"
            packed=$((packed + 1))
        fi
    done
    echo "  copied: $packed modded BMP(s)"
    "$PYTHON" "$PROJROOT/tools/pack_assets.py" "$PROJROOT/$TARGET_SRC"
    echo ""
    echo "=== Done! Rebuild $TARGET_SRC/Project1.vbp with VB5 to produce an updated EXE ==="
else
    echo "--- [3/3] Deploying external assets to $GAME_DIR/assets/ ---"
    mkdir -p "$GAME_DIR/assets/images"
    deployed=0
    for f in "$SCRIPT_DIR/output/"*.bmp; do
        if [ -f "$f" ]; then
            base=$(basename "$f")
            cp "$f" "$GAME_DIR/assets/images/$base"
            echo "  images/$base (modded)"
            deployed=$((deployed + 1))
        fi
    done
    # Copy any unmodded original files across icons/, images/, sfx/, midi/
    for subdir in icons images sfx midi; do
        if [ -d "$SRC_ASSETS/$subdir" ]; then
            mkdir -p "$GAME_DIR/assets/$subdir"
            for f in "$SRC_ASSETS/$subdir/"*; do
                [ -f "$f" ] || continue
                base=$(basename "$f")
                if [ ! -f "$GAME_DIR/assets/$subdir/$base" ]; then
                    cp "$f" "$GAME_DIR/assets/$subdir/$base"
                    echo "  $subdir/$base (unmodded)"
                fi
            done
        fi
    done
    echo "  modded images deployed: $deployed"
    echo ""
    echo "=== Done! Launch the game (built with patches/external-assets.patch) ==="
    echo "  cd $GAME_DIR && wine explorer /desktop=llkplay,800x600 D4s.exe"
fi
