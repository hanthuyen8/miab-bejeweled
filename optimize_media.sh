#!/usr/bin/env bash
#
# Losslessly shrinks the PNGs under media/. Run it after re-exporting any image
# from Photoshop — an export undoes the optimisation, and nothing else catches
# that, so the shipped build silently grows back.
#
# Two passes, both lossless:
#   1. Drop the alpha channel from images that are fully opaque. Photoshop
#      exports RGBA even when nothing is transparent, and that dead channel is
#      a quarter of the file. Only done when EVERY pixel is alpha=255, so images
#      that really use transparency (the logo, the atlas) keep it.
#   2. oxipng — re-encodes the deflate stream and picks better filters. It never
#      alters image data.
#
# Deliberately NOT lossy: no pngquant, no JPEG. The art has gradients that band
# under palette quantisation. Also does NOT resize — that is per-image intent
# (see mainMenuLogo, shipped at the 628px it is drawn at, master kept in
# assets/photoshop/).
#
# Safe to re-run: already-optimised files simply come out unchanged.
#
# Requires: oxipng (brew install oxipng), Pillow (pip3 install Pillow).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

command -v oxipng >/dev/null 2>&1 || {
    echo "error: oxipng not found — brew install oxipng" >&2; exit 1; }
python3 -c "import PIL" 2>/dev/null || {
    echo "error: Pillow not found — pip3 install Pillow" >&2; exit 1; }

total_before=0
total_after=0

while IFS= read -r png; do
    before=$(stat -f%z "$png")

    # Pass 1: strip alpha, but only when it carries no information at all
    stripped=$(python3 - "$png" <<'PY'
import sys
from PIL import Image

path = sys.argv[1]
im = Image.open(path)
if im.mode in ("RGBA", "LA"):
    alpha = im.getchannel("A")
    if alpha.getextrema() == (255, 255):
        im.convert("RGB" if im.mode == "RGBA" else "L").save(path)
        print("yes")
PY
)

    # Pass 2: lossless re-encode
    oxipng -o max --strip safe -q "$png"

    after=$(stat -f%z "$png")
    total_before=$((total_before + before))
    total_after=$((total_after + after))

    if [ "$before" -ne "$after" ]; then
        note=""
        [ -n "$stripped" ] && note="  (alpha removed)"
        printf "  %-42s %6d KB -> %6d KB%s\n" \
            "${png#"$REPO_ROOT"/}" $((before / 1024)) $((after / 1024)) "$note"
    fi
done < <(find "$REPO_ROOT/media" -name "*.png" | sort)

echo
printf "total  %d KB -> %d KB\n" $((total_before / 1024)) $((total_after / 1024))

if [ "$total_before" -ne "$total_after" ]; then
    echo
    echo "media/ changed — remember to delete build-web/seajeweled.{data,js,wasm,html}"
    echo "before ./build.sh, or the preloaded data file will still be the old one."
fi
