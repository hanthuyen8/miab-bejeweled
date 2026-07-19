#!/usr/bin/env python3
"""Regenerate the GemShine figures used by docs/gemshine-explained.md.

This mirrors src/GemShine.cpp in Python so the diagrams can be rebuilt without
running the game or dumping VRAM. It reads the real gem sprites out of
media/atlas.png, so it stays honest about the actual art.

The maths below is a transcription of GemShine.cpp -- if that file changes
(band constants, streak angle, frame count, blend mode) update this too and
re-run, or the docs will quietly drift from the code.

Run from the repo root:  python3 docs/images/gemshine_figures.py
"""

import json
import math
import os

import numpy as np
from PIL import Image

OUT_DIR = "docs/images"

# --- constants transcribed from GemShine.h / GemShine.cpp -------------------

CELL = 65           # kCellSize
STREAK_W = 60       # kStreakWidth
FRAME_COUNT = 24    # kFrameCount
ANGLE_DEG = 80      # kStreakAngleDegrees

# kMainOffset/Sigma/Peak and kEdgeOffset/Sigma/Peak
MAIN = (0.0, 9.0, 0.62)
EDGE = (19.0, 4.0, 0.40)

# GemShine::streakLean()
LEAN = -1.0 / math.tan(math.radians(ANGLE_DEG))

# kGemSprites, in GemShine::Gem order
GEM_SPRITES = [
    "gemWhite.png", "gemRed.png", "gemPurple.png", "gemOrange.png",
    "gemGreen.png", "gemYellow.png", "gemBlue.png",
]


def build_streak():
    """createStreakTexture(): two gaussian bands, leaning, intensity in RGB."""
    xs = np.arange(STREAK_W)[None, :]
    ys = np.arange(CELL)[:, None]

    # Distance from the streak's axis, which leans as y grows
    u = (xs - STREAK_W / 2.0) - (ys - CELL / 2.0) * LEAN

    def band(offset, sigma, peak):
        d = (u - offset) / sigma
        return peak * np.exp(-d * d)

    intensity = np.clip(band(*MAIN) + band(*EDGE), 0.0, 1.0)
    return (intensity * 255).astype(np.uint8)


def composite_cell(gem, streak, frame):
    """One sheet cell: the gem, then the streak through maskedAdditiveBlend().

        colour = streak * dstAlpha + colour
        alpha  = alpha

    dstAlpha is the gem's own alpha, so the highlight is confined to the gem
    silhouette instead of spilling onto the board behind it.
    """
    out = gem.copy()

    travel = frame / (FRAME_COUNT - 1)
    streak_x = -STREAK_W + int((CELL + STREAK_W) * travel)

    for px in range(CELL):
        u = px - streak_x
        if 0 <= u < STREAK_W:                       # outside == clipped away
            dst_alpha = out[:, px, 3] / 255.0
            add = streak[:, u].astype(np.float64) * dst_alpha
            out[:, px, :3] = np.clip(out[:, px, :3] + add[:, None], 0, 255)

    return out


def checkerboard(rgba, light=60, dark=45, size=8):
    """Composite over a checkerboard so transparent regions are visible."""
    h, w = rgba.shape[:2]
    tile = (np.indices((h, w)).sum(0) // size) % 2
    base = np.dstack([np.where(tile == 0, light, dark)] * 3
                     + [np.full((h, w), 255)]).astype(np.float64)

    a = rgba[:, :, 3:4] / 255.0
    out = base.copy()
    out[:, :, :3] = rgba[:, :, :3] * a + base[:, :, :3] * (1 - a)
    return out


def main():
    atlas = Image.open("media/atlas.png").convert("RGBA")
    frames = json.load(open("media/atlas.json"))["frames"]

    os.makedirs(OUT_DIR, exist_ok=True)
    streak = build_streak()

    # 1. the streak on its own, magnified so the two bands are legible
    Image.fromarray(
        np.dstack([streak] * 3 + [np.full_like(streak, 255)]), "RGBA"
    ).resize((STREAK_W * 3, CELL * 3), Image.NEAREST).save(
        f"{OUT_DIR}/gemshine_streak.png")

    # 2/3. the full sheet, exactly what bake() renders
    sheet = np.zeros((len(GEM_SPRITES) * CELL, FRAME_COUNT * CELL, 4))

    for row, sprite in enumerate(GEM_SPRITES):
        f = frames[sprite]["frame"]
        box = (f["x"], f["y"], f["x"] + f["w"], f["y"] + f["h"])
        gem = np.asarray(
            atlas.crop(box).resize((CELL, CELL))).astype(np.float64)

        for frame in range(FRAME_COUNT):
            cell = composite_cell(gem, streak, frame)
            sheet[row * CELL:(row + 1) * CELL,
                  frame * CELL:(frame + 1) * CELL] = cell

    Image.fromarray(sheet.astype(np.uint8), "RGBA").save(
        f"{OUT_DIR}/gemshine_sheet.png")
    Image.fromarray(checkerboard(sheet).astype(np.uint8), "RGBA").save(
        f"{OUT_DIR}/gemshine_sheet_checker.png")

    # 4. one gem's row across the whole sweep
    blue = len(GEM_SPRITES) - 1
    row_img = sheet[blue * CELL:(blue + 1) * CELL]
    Image.fromarray(row_img.astype(np.uint8), "RGBA").save(
        f"{OUT_DIR}/gemshine_sweep_row.png")

    # 5. a few frames magnified, on a dark backdrop
    picks, zoom, gap = [0, 8, 11, 14, 17, 23], 4, 10
    strip = Image.new(
        "RGBA",
        (len(picks) * CELL * zoom + (len(picks) - 1) * gap, CELL * zoom),
        (25, 22, 35, 255))

    x = 0
    for frame in picks:
        cell = Image.fromarray(
            row_img[:, frame * CELL:(frame + 1) * CELL].astype(np.uint8), "RGBA")
        strip.alpha_composite(cell.resize((CELL * zoom, CELL * zoom), Image.NEAREST), (x, 0))
        x += CELL * zoom + gap

    strip.save(f"{OUT_DIR}/gemshine_frames_zoom.png")

    print(f"sheet {FRAME_COUNT * CELL}x{len(GEM_SPRITES) * CELL}"
          f" ({FRAME_COUNT} frames x {len(GEM_SPRITES)} gems) -> {OUT_DIR}/")
    print(f"zoomed frames: {picks}")


if __name__ == "__main__":
    main()
