#ifndef _BITMAPNUMBER_H_
#define _BITMAPNUMBER_H_

#include <string>

#include "go_window.h"
#include "go_image.h"
#include "go_textureatlas.h"
#include "Assets.h"

/**
 * Draws numeric strings ("0"-"9" and ':') using glyphs from the shared texture
 * atlas. This replaces per-frame SDL_ttf rasterization for the score/time
 * readouts: the digits are plain atlas draws, so they batch with the rest of
 * the board sprites and no longer break the batch by z-order.
 *
 * Glyphs are baked white at BakedSize px (see texture-packer/generate_glyphs.py);
 * colour is applied via colorMod and size via a scale = fontSize / BakedSize
 * (only ever scaling down, so no blur). The LCD font is monospace, so every
 * glyph advances by the same width.
 */
class BitmapNumber {
public:

    /// Em size the glyphs were rendered at.
    static constexpr int BakedSize = 72;

    void loadResources(GoSDL::Window * window)
    {
        GoSDL::TextureAtlas atlas;
        atlas.load(window, Assets::AtlasImage, Assets::AtlasData);

        atlas.setImage(mDigits[0], Assets::Sprite::Digit0);
        atlas.setImage(mDigits[1], Assets::Sprite::Digit1);
        atlas.setImage(mDigits[2], Assets::Sprite::Digit2);
        atlas.setImage(mDigits[3], Assets::Sprite::Digit3);
        atlas.setImage(mDigits[4], Assets::Sprite::Digit4);
        atlas.setImage(mDigits[5], Assets::Sprite::Digit5);
        atlas.setImage(mDigits[6], Assets::Sprite::Digit6);
        atlas.setImage(mDigits[7], Assets::Sprite::Digit7);
        atlas.setImage(mDigits[8], Assets::Sprite::Digit8);
        atlas.setImage(mDigits[9], Assets::Sprite::Digit9);
        atlas.setImage(mColon, Assets::Sprite::Colon);

        mGlyphWidth = mDigits[0].getWidth();
    }

    /// Pixel width of `text` rendered at the given font size.
    int width(const std::string & text, int fontSize) const
    {
        return int(text.size() * mGlyphWidth * scaleFor(fontSize));
    }

    /// Draw `text` with its left edge at (x, y), matching a TTF render at
    /// `fontSize`. Only '0'-'9' and ':' are drawn; other chars advance blank.
    void draw(const std::string & text, int x, int y, int z, int fontSize,
              SDL_Color color, Uint8 alpha = 255)
    {
        double scale = scaleFor(fontSize);
        int advance = int(mGlyphWidth * scale);

        for (char c : text)
        {
            GoSDL::Image * glyph = glyphFor(c);
            if (glyph)
            {
                glyph->draw(x, y, z, scale, scale, 0, alpha, color);
            }
            x += advance;
        }
    }

private:

    double scaleFor(int fontSize) const
    {
        return double(fontSize) / BakedSize;
    }

    GoSDL::Image * glyphFor(char c)
    {
        if (c >= '0' && c <= '9') return &mDigits[c - '0'];
        if (c == ':') return &mColon;
        return nullptr;
    }

    GoSDL::Image mDigits[10];
    GoSDL::Image mColon;
    int mGlyphWidth = 0;
};

#endif /* _BITMAPNUMBER_H_ */
