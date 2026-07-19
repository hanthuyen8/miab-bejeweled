#!/usr/bin/env bash
#
# Regenerate the bitmap-font glyphs and re-publish the texture atlas.
#
# Produces:
#   media/atlas.png    packed sprites + glyphs
#   media/atlas.json   frame table
#   media/fonts.json   font metrics (written by generate_glyphs.py)
#
# Requires TexturePacker's CLI (installed with TexturePacker.app) and Pillow.
# Run it after changing anything in assets/sprites/ or assets/fonts/, or after
# editing CHARSET / FACES in generate_glyphs.py.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v TexturePacker >/dev/null 2>&1; then
    echo "error: TexturePacker CLI not found on PATH." >&2
    echo "       Install TexturePacker.app, then enable its command-line tool." >&2
    exit 1
fi

echo "==> baking glyphs"
python3 "$REPO_ROOT/texture-packer/generate_glyphs.py"

echo
echo "==> packing atlas"
TexturePacker "$REPO_ROOT/texture-packer/atlas.tps"

echo
echo "==> compressing atlas"
# Lossless only — the atlas holds the gem art, so every pixel has to survive
# byte for byte. oxipng just re-encodes the deflate stream and picks better
# filters; it never touches the image data. (pngquant would compress this to
# roughly half the size, but it quantises to a 256-colour palette, which bands
# the gem gradients.)
#
# This has to live here rather than being run once by hand, because every
# Publish overwrites media/atlas.png and would silently undo it.
if command -v oxipng >/dev/null 2>&1; then
    before=$(stat -f%z "$REPO_ROOT/media/atlas.png")
    oxipng -o max --strip safe -q "$REPO_ROOT/media/atlas.png"
    after=$(stat -f%z "$REPO_ROOT/media/atlas.png")
    echo "    $((before / 1024)) KB -> $((after / 1024)) KB"
else
    echo "    oxipng not found, shipping the atlas uncompressed" >&2
fi

echo
echo "==> done"
python3 - "$REPO_ROOT" <<'EOF'
import json, sys, os
root = sys.argv[1]
meta = json.load(open(os.path.join(root, "media/atlas.json")))
size = meta["meta"]["size"]
frames = meta["frames"]
png = os.path.getsize(os.path.join(root, "media/atlas.png"))
print(f"    atlas.png  {size['w']}x{size['h']}  {png/1024:.0f} KB  {len(frames)} frames")
EOF
