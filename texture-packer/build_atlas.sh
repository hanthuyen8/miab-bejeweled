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
