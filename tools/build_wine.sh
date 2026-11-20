#!/usr/bin/env bash
# Portable Wine VB5 builder for KAWAI LianLianKan 2 (v1.02 & v1.00-cn).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

VERSION="all"
EXTERNAL=0
OUT_DIR=""

usage() {
    cat <<'EOF'
Usage: build_wine.sh [--version 1.02|1.00-cn|all] [--external] [--out DIR]

Options:
  --version VER   Build 1.02, 1.00-cn, or all (default: all)
  --external      Build after patches/external-assets.patch (defaults output to
                  src/ or src-llkcn/ right beside assets/ unless --out is set)
  --out DIR       Output directory (integrated default: ./build)
  -h, --help      Show this help message

Environment:
  WINEPREFIX      Wine prefix containing VB5 Enterprise (default: ~/.wine)
  VB5_EXE         Optional path to VB5.EXE (auto-detected under drive_c if unset)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            VERSION="${2:?Missing value for --version}"
            shift 2
            ;;
        --external)
            EXTERNAL=1
            shift
            ;;
        --out)
            OUT_DIR="${2:?Missing value for --out}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

case "$VERSION" in
    1.02|1.00-cn|all) ;;
    *)
        echo "Invalid --version: $VERSION (expected 1.02, 1.00-cn, or all)" >&2
        exit 1
        ;;
esac

export WINEPREFIX="${WINEPREFIX:-$HOME/.wine}"
if [[ ! -d "$WINEPREFIX" ]]; then
    echo "ERROR: WINEPREFIX does not exist: $WINEPREFIX" >&2
    exit 1
fi

if [[ -z "${VB5_EXE:-}" ]]; then
    for cand in \
        "$WINEPREFIX/drive_c/Program Files/DevStudio/VB/VB5.EXE" \
        "$WINEPREFIX/drive_c/Program Files (x86)/DevStudio/VB/VB5.EXE" \
        "$WINEPREFIX/drive_c/vb5/VB5.EXE"; do
        if [[ -f "$cand" ]]; then
            VB5_EXE="$cand"
            break
        fi
    done
fi
if [[ -z "${VB5_EXE:-}" || ! -f "$VB5_EXE" ]]; then
    echo "ERROR: Could not locate VB5.EXE in $WINEPREFIX (set VB5_EXE env var)." >&2
    exit 1
fi

# Prefer Traditional Chinese UTF-8 locale if installed for cleaner Win32 ANSI mapping
if command -v locale >/dev/null 2>&1; then
    for loc in zh_TW.UTF-8 zh_TW.utf8 C.UTF-8; do
        if locale -a 2>/dev/null | grep -qx "$loc"; then
            export LC_ALL="$loc"
            break
        fi
    done
fi

build_one() {
    local ver="$1"
    local proj_sub="$2"
    local exe_name="$3"
    local proj_dir="$ROOT_DIR/$proj_sub"
    local vbp_unix="$proj_dir/Project1.vbp"

    if [[ ! -f "$vbp_unix" ]]; then
        echo "ERROR: Missing project file: $vbp_unix" >&2
        return 1
    fi

    if grep -qP '"\xb0\xb1\xa4\xee"' "$proj_dir/Form_Main.frm" 2>/dev/null || \
       python3 -c 'import sys; sys.exit(0 if b"\xb0\xb1\xa4\xee" in open(sys.argv[1], "rb").read() else 1)' "$proj_dir/Form_Main.frm"; then
        echo "WARNING: $proj_sub/Form_Main.frm still contains raw Big5 literals; apply patches/wine-compat.patch for Wine." >&2
    fi

    if [[ "$EXTERNAL" -eq 0 ]]; then
        for req in Form_Main.frx Form_Explain.frx Form_Author.frx Project1.res; do
            if [[ ! -f "$proj_dir/$req" ]]; then
                echo "ERROR: Missing $proj_sub/$req. Run python3 tools/extract_assets.py first (or pass --external after external-assets.patch)." >&2
                return 1
            fi
        done
    fi

    local dest_dir
    if [[ -n "$OUT_DIR" ]]; then
        dest_dir="$OUT_DIR"
    elif [[ "$EXTERNAL" -eq 1 ]]; then
        dest_dir="$proj_dir"
    else
        dest_dir="$ROOT_DIR/build"
    fi
    mkdir -p "$dest_dir"

    local out_unix="$dest_dir/$exe_name"
    local log_unix="$dest_dir/build_${ver}.log"
    rm -f "$out_unix" "$log_unix"

    local vbp_win out_win log_win
    vbp_win="$(WINEPREFIX="$WINEPREFIX" winepath -w "$vbp_unix")"
    out_win="$(WINEPREFIX="$WINEPREFIX" winepath -w "$out_unix")"
    log_win="$(WINEPREFIX="$WINEPREFIX" winepath -w "$log_unix")"

    echo "[build_wine] Building v$ver ($proj_sub) -> $out_unix"
    WINEDEBUG=-all WINEPREFIX="$WINEPREFIX" wine "$VB5_EXE" /make "$vbp_win" "$out_win" /out "$log_win"
    WINEPREFIX="$WINEPREFIX" wineserver -w

    if [[ -f "$log_unix" ]]; then
        cat "$log_unix"
    fi
    if [[ ! -s "$out_unix" ]]; then
        echo "ERROR: Expected output executable was not produced: $out_unix" >&2
        return 1
    fi
    rm -f "$log_unix"
    ls -lh "$out_unix"
}

if [[ "$VERSION" == "1.02" || "$VERSION" == "all" ]]; then
    build_one "1.02" "src" "D4S.exe"
fi
if [[ "$VERSION" == "1.00-cn" || "$VERSION" == "all" ]]; then
    build_one "1.00-cn" "src-llkcn" "Lianliankan2.exe"
fi
