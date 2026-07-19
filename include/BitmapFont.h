#ifndef _BITMAPFONT_H_
#define _BITMAPFONT_H_

#include <map>
#include <string>
#include <vector>

#include <SDL.h>

#include "go_image.h"

namespace GoSDL {
    class Window;
}

/**
 * Bitmap-font text rendering from the shared texture atlas (Step 5).
 *
 * Every glyph the game draws is baked into media/atlas.png ahead of time (see
 * texture-packer/generate_glyphs.py), so drawing text is just a run of atlas
 * quads: nothing is rasterized at runtime, no per-string SDL_Texture is
 * allocated, and the glyphs batch with the gems and UI art instead of forcing
 * their own draw calls.
 *
 * Glyphs are baked white and tinted at draw time via colorMod, which is also
 * how the drop shadow is drawn (the same run, tinted black and offset).
 *
 * Only the characters in generate_glyphs.py's CHARSET exist. Anything else --
 * including any non-ASCII byte from a gettext translation -- is skipped, and
 * advances nothing. Text is treated as bytes, not UTF-8.
 */

/// One baked (face, size) sheet: all glyphs rendered at a single em size.
class BitmapFontSheet {
public:

    struct Glyph {
        GoSDL::Image image;
        int advance = 0;   ///< pen movement, which is NOT the sprite width
        int xoffset = 0;   ///< sprite position relative to the pen
        int yoffset = 0;   ///< sprite position relative to the line box top
    };

    /// Returns nullptr for characters this sheet does not carry.
    const Glyph * find(char c) const;

    int bakedSize = 0;
    int ascent = 0;
    int descent = 0;
    int lineHeight = 0;
    int spaceAdvance = 0;

private:
    friend class BitmapFontAtlas;
    std::map<unsigned char, Glyph> mGlyphs;
};

/// Owns every sheet. Loaded once and handed out to whoever draws text.
class BitmapFontAtlas {
public:

    /// Parses media/fonts.json and points each glyph at its atlas frame.
    /// Must be called once, before any BitmapFont is used.
    bool load(GoSDL::Window * window,
              const std::string & atlasImagePath,
              const std::string & atlasDataPath,
              const std::string & metricsPath);

    /// Smallest baked sheet that is at least `fontSize`, so glyphs are scaled
    /// down but never up. Falls back to the largest sheet of the face.
    /// Returns nullptr if the face is unknown.
    const BitmapFontSheet * sheetFor(const std::string & face, int fontSize) const;

private:
    /// Per face, sheets sorted by baked size ascending.
    std::map<std::string, std::vector<BitmapFontSheet> > mFaces;
};

/**
 * A (face, size) view onto the atlas. Cheap to copy and to keep as a member;
 * it holds no resources of its own, so the BitmapFontAtlas it points at must
 * outlive it (Game owns the atlas and outlives every state).
 *
 * Coordinates match what GoSDL::Font used to produce: (x, y) is the top-left
 * of the line box, and the shadow variants place the text at (x, y) with the
 * shadow offset behind it, exactly as the old composited surfaces did.
 */
class BitmapFont {
public:

    BitmapFont() { }
    BitmapFont(const BitmapFontAtlas * atlas, const std::string & face, int fontSize);

    void setAll(const BitmapFontAtlas * atlas, const std::string & face, int fontSize);

    /// True once a valid face/size has been set.
    bool isValid() const { return mSheet != nullptr; }

    int getTextWidth(const std::string & text) const;

    /// Height of one line, i.e. what the old rendered text image measured.
    int getHeight() const;

    int getAscent() const;

    void draw(const std::string & text, int x, int y, int z,
              SDL_Color color = {255, 255, 255, 255}, Uint8 alpha = 255) const;

    void drawWithShadow(const std::string & text, int x, int y, int z,
                        SDL_Color color = {255, 255, 255, 255},
                        int shadowX = 0, int shadowY = 2,
                        SDL_Color shadowColor = {0, 0, 0, 128},
                        Uint8 alpha = 255) const;

    /// Greedy word wrap to `maxWidth` pixels, honouring existing '\n' breaks.
    /// Wrap once at load time and keep the lines; do not call this per frame.
    std::vector<std::string> wrapText(const std::string & text, int maxWidth) const;

private:

    const BitmapFontSheet * mSheet = nullptr;
    int mFontSize = 0;
    double mScale = 1.0;
};

#endif /* _BITMAPFONT_H_ */
