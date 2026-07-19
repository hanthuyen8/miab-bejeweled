#!/usr/bin/env python3
"""Render digit glyphs (0-9 and ':') from Quicksand-Bold.ttf into individual
white PNGs for the texture atlas (Step 4 — bitmap font).

Design:
- White glyphs only: colour is applied at draw time via SDL_SetTextureColorMod
  (colorMod multiplies, so white can be tinted to any colour, incl. the black
  drop shadow). See docs.
- One size (the largest used in-game, 72px): smaller usages scale down via the
  destination rect (linear filtering is already enabled).
- Fixed canvas height (font ascent+descent) with the glyph drawn at the
  ascender line, so every glyph shares the same baseline and they line up when
  drawn at the same y. Width = the glyph's advance, so stepping by the sprite
  width gives correct spacing with no extra tracking.

Output: assets/sprites/digit0.png .. digit9.png, colon.png
Run from the repo root:  python3 texture-packer/generate_glyphs.py
Then add the new PNGs to atlas.tps and re-publish the atlas.
"""

import math
import os

from PIL import Image, ImageDraw, ImageFont

FONT_PATH = "assets/fonts/Quicksand-Bold.ttf"
SIZE = 72
OUT_DIR = "assets/glyphs/lcd"

GLYPHS = {
    "0": "digit0", "1": "digit1", "2": "digit2", "3": "digit3", "4": "digit4",
    "5": "digit5", "6": "digit6", "7": "digit7", "8": "digit8", "9": "digit9",
    ":": "colon",
}


def main():
    font = ImageFont.truetype(FONT_PATH, SIZE)
    ascent, descent = font.getmetrics()
    height = ascent + descent

    os.makedirs(OUT_DIR, exist_ok=True)

    for ch, name in GLYPHS.items():
        advance = max(1, math.ceil(font.getlength(ch)))
        img = Image.new("RGBA", (advance, height), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        # Default anchor "la" places the ascender line at y=0 -> shared baseline
        draw.text((0, 0), ch, font=font, fill=(255, 255, 255, 255))
        img.save(os.path.join(OUT_DIR, name + ".png"))
        print(f"{name+'.png':14s} {advance:3d}x{height}")

    print(f"\nDone. {len(GLYPHS)} glyphs @ {SIZE}px (height={height}) -> {OUT_DIR}/")


if __name__ == "__main__":
    main()
