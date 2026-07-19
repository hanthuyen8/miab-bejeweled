#!/usr/bin/env python3
"""Bake bitmap glyph sheets for the in-game fonts (Step 5 — bitmap font atlas).

Replaces runtime SDL_ttf rasterization: every glyph is baked once into the
shared texture atlas, so text draws are plain atlas quads that batch with the
rest of the sprites and cost no rasterization at all.

Design
------
- White glyphs only. Colour is applied at draw time via SDL_SetTextureColorMod
  (colorMod multiplies, so white tints to any colour, including the black drop
  shadow).
- Two baked sizes per face (40 and 72). The runtime picks the 40px sheet for
  text <= 40px and the 72px sheet above that, so a glyph is never downscaled by
  more than ~1.55x and never upscaled. Baking a single 72px sheet would mean a
  3.6x downscale for 20px button captions, which reads as blurry.
- Shared baseline: every glyph in a sheet uses the same canvas height
  (ascent + descent) with the ascender line at y=0, so drawing a run at a
  common y lines the glyphs up with no per-glyph vertical math.
- Canvas width is the glyph's advance, padded outwards only where the ink
  actually overflows it (negative left bearing, or ink wider than the advance).
  The padding is reported back as xoffset/yoffset so the runtime can draw the
  ink correctly while still stepping by the true advance.

Filenames use the codepoint (`menu72_u0041.png`), never the character itself:
macOS filesystems are case-insensitive, so `a.png` and `A.png` would collide.

Output
------
  assets/glyphs/<face><size>/<face><size>_u<CP>.png   glyph images (for the atlas)
  media/fonts.json                                    metrics consumed at runtime

Run from the repo root:  python3 texture-packer/generate_glyphs.py
Then re-publish the atlas:  texture-packer/build_atlas.sh
"""

import json
import math
import os
import shutil

from PIL import Image, ImageDraw, ImageFont

# The character set the game draws. Anything outside this set renders blank.
CHARSET = (
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "!;%:?*()_+-=.,\"'@#&[]"
)

# Digits and ':' only — the LCD score/time readouts, which are deliberately a
# heavier weight than the surrounding UI text.
LCD_CHARSET = "0123456789:"

GLYPH_ROOT = "assets/glyphs"
METRICS_PATH = "media/fonts.json"

# face -> (source ttf, baked sizes, charset)
#
# Sizes are driven by what the game actually asks for:
#   menu   (Quicksand SemiBold) 20,23,26,30,31 -> 40 | 48,60,64,72 -> 72
#   normal (Miso Regular)       28,35          -> 40   (never larger)
#   lcd    (Quicksand Bold)     30             -> 40 | 60           -> 72
FACES = {
    "menu": ("assets/fonts/Quicksand-SemiBold.ttf", [40, 72], CHARSET),
    "normal": ("assets/fonts/Miso-Regular.ttf", [40], CHARSET),
    "lcd": ("assets/fonts/Quicksand-Bold.ttf", [40, 72], LCD_CHARSET),
}


def bake_sheet(face, size, font_path, charset):
    """Render one (face, size) sheet. Returns its metrics dict."""
    font = ImageFont.truetype(font_path, size)
    ascent, descent = font.getmetrics()
    height = ascent + descent

    out_dir = os.path.join(GLYPH_ROOT, f"{face}{size}")
    # Wipe the folder so glyphs dropped from CHARSET don't linger in the atlas.
    shutil.rmtree(out_dir, ignore_errors=True)
    os.makedirs(out_dir, exist_ok=True)

    glyphs = []
    padded = []

    for ch in charset:
        advance = max(1, math.ceil(font.getlength(ch)))

        # Ink bounds relative to the anchor origin (ascender line at y=0).
        x0, y0, x1, y1 = font.getbbox(ch)

        # Grow the canvas only where the ink escapes the advance box.
        pad_left = max(0, -x0)
        pad_right = max(0, x1 - advance)
        pad_top = max(0, -y0)
        pad_bottom = max(0, y1 - height)

        if pad_left or pad_right or pad_top or pad_bottom:
            padded.append(ch)

        canvas_w = pad_left + advance + pad_right
        canvas_h = pad_top + height + pad_bottom

        img = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        # Anchor "la" = left edge, ascender line -> shared baseline across glyphs.
        draw.text((pad_left, pad_top), ch, font=font, fill=(255, 255, 255, 255))

        sprite = f"{face}{size}_u{ord(ch):04X}.png"
        img.save(os.path.join(out_dir, sprite))

        glyphs.append({
            "cp": ord(ch),
            "sprite": sprite,
            "advance": advance,
            # Where to draw the sprite relative to the pen position / baseline.
            "xoffset": -pad_left,
            "yoffset": -pad_top,
        })

    space_advance = max(1, round(font.getlength(" ")))

    note = f"  padded: {''.join(padded)}" if padded else ""
    print(f"  {face}{size}: {len(glyphs):3d} glyphs  h={height:3d} "
          f"asc={ascent} desc={descent} space={space_advance}{note}")

    return {
        "size": size,
        "ascent": ascent,
        "descent": descent,
        "lineHeight": height,
        "spaceAdvance": space_advance,
        "glyphs": glyphs,
    }


def main():
    os.makedirs(os.path.dirname(METRICS_PATH), exist_ok=True)

    faces = {}
    total_area = 0

    for face, (font_path, sizes, charset) in FACES.items():
        print(f"{face}  <- {font_path}")
        sheets = [bake_sheet(face, size, font_path, charset) for size in sizes]
        faces[face] = {"sheets": sheets}

        for sheet in sheets:
            for g in sheet["glyphs"]:
                total_area += g["advance"] * sheet["lineHeight"]

    with open(METRICS_PATH, "w") as f:
        json.dump({"faces": faces}, f, indent=1)

    sheet_count = sum(len(f["sheets"]) for f in faces.values())
    glyph_count = sum(len(s["glyphs"]) for f in faces.values() for s in f["sheets"])
    print(f"\n{glyph_count} glyphs across {sheet_count} sheets")
    print(f"~{total_area} px^2 of glyph area (~{int(math.sqrt(total_area))}px square)")
    print(f"metrics -> {METRICS_PATH}")


if __name__ == "__main__":
    main()
