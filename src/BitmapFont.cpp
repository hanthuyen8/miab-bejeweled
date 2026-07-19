#include "BitmapFont.h"

#include "go_textureatlas.h"
#include "go_window.h"
#include "Util.h"
#include "log.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string_view>
#include <json/json.h>

// ---------------------------------------------------------------- sheet ----

const BitmapFontSheet::Glyph * BitmapFontSheet::find(char c) const
{
    auto it = mGlyphs.find(static_cast<unsigned char>(c));
    return it == mGlyphs.end() ? nullptr : &it->second;
}

// ---------------------------------------------------------------- atlas ----

bool BitmapFontAtlas::load(GoSDL::Window * window,
                           const std::string & atlasImagePath,
                           const std::string & atlasDataPath,
                           const std::string & metricsPath)
{
    mFaces.clear();

    GoSDL::TextureAtlas atlas;
    if (!atlas.load(window, atlasImagePath, atlasDataPath))
    {
        lDEBUG << "BitmapFontAtlas: could not load the texture atlas";
        return false;
    }

    std::ifstream stream(getBasePath() + metricsPath, std::ios::in);
    if (!stream.is_open())
    {
        lDEBUG << "BitmapFontAtlas: could not open font metrics: " << metricsPath;
        return false;
    }

    Json::CharReaderBuilder builder;
    Json::Value root;
    JSONCPP_STRING errs;

    if (!parseFromStream(builder, stream, &root, &errs))
    {
        lDEBUG << "BitmapFontAtlas: could not parse font metrics: " << errs;
        return false;
    }

    const Json::Value & faces = root["faces"];

    for (const auto & faceName : faces.getMemberNames())
    {
        std::vector<BitmapFontSheet> sheets;

        for (const Json::Value & sheetJson : faces[faceName]["sheets"])
        {
            BitmapFontSheet sheet;
            sheet.bakedSize    = sheetJson["size"].asInt();
            sheet.ascent       = sheetJson["ascent"].asInt();
            sheet.descent      = sheetJson["descent"].asInt();
            sheet.lineHeight   = sheetJson["lineHeight"].asInt();
            sheet.spaceAdvance = sheetJson["spaceAdvance"].asInt();

            for (const Json::Value & glyphJson : sheetJson["glyphs"])
            {
                BitmapFontSheet::Glyph glyph;
                glyph.advance = glyphJson["advance"].asInt();
                glyph.xoffset = glyphJson["xoffset"].asInt();
                glyph.yoffset = glyphJson["yoffset"].asInt();

                const std::string sprite = glyphJson["sprite"].asString();

                // A glyph missing from the atlas means the two generated
                // artefacts are out of sync; skip it rather than drawing the
                // whole atlas in its place.
                if (!atlas.setImage(glyph.image, sprite))
                {
                    lDEBUG << "BitmapFontAtlas: glyph missing from atlas: " << sprite;
                    continue;
                }

                const int codepoint = glyphJson["cp"].asInt();
                if (codepoint < 0 || codepoint > 255)
                {
                    lDEBUG << "BitmapFontAtlas: non-byte codepoint ignored: " << codepoint;
                    continue;
                }

                sheet.mGlyphs[static_cast<unsigned char>(codepoint)] = std::move(glyph);
            }

            sheets.push_back(std::move(sheet));
        }

        // sheetFor() walks these in order and takes the first that fits.
        std::sort(sheets.begin(), sheets.end(),
                  [](const BitmapFontSheet & a, const BitmapFontSheet & b) {
                      return a.bakedSize < b.bakedSize;
                  });

        mFaces[faceName] = std::move(sheets);
    }

    return true;
}

const BitmapFontSheet * BitmapFontAtlas::sheetFor(const std::string & face, int fontSize) const
{
    auto it = mFaces.find(face);
    if (it == mFaces.end() || it->second.empty())
    {
        lDEBUG << "BitmapFontAtlas: unknown face: " << face;
        return nullptr;
    }

    const std::vector<BitmapFontSheet> & sheets = it->second;

    for (const BitmapFontSheet & sheet : sheets)
    {
        if (fontSize <= sheet.bakedSize)
            return &sheet;
    }

    // Asked for more than we baked: use the largest and upscale.
    return &sheets.back();
}

// ----------------------------------------------------------------- font ----

BitmapFont::BitmapFont(const BitmapFontAtlas * atlas, const std::string & face, int fontSize)
{
    setAll(atlas, face, fontSize);
}

void BitmapFont::setAll(const BitmapFontAtlas * atlas, const std::string & face, int fontSize)
{
    mSheet = atlas ? atlas->sheetFor(face, fontSize) : nullptr;
    mFontSize = fontSize;
    mScale = mSheet ? double(fontSize) / mSheet->bakedSize : 1.0;
}

int BitmapFont::getTextWidth(const std::string & text) const
{
    if (!mSheet) return 0;

    // Accumulated in font units and scaled once, so a long run does not drift
    // from the pen positions used by draw().
    int advance = 0;

    for (char c : text)
    {
        if (c == ' ')
        {
            advance += mSheet->spaceAdvance;
            continue;
        }

        if (const BitmapFontSheet::Glyph * glyph = mSheet->find(c))
            advance += glyph->advance;
    }

    return int(std::lround(advance * mScale));
}

int BitmapFont::getHeight() const
{
    return mSheet ? int(std::lround(mSheet->lineHeight * mScale)) : 0;
}

int BitmapFont::getAscent() const
{
    return mSheet ? int(std::lround(mSheet->ascent * mScale)) : 0;
}

void BitmapFont::draw(const std::string & text, int x, int y, int z,
                      SDL_Color color, Uint8 alpha) const
{
    if (!mSheet) return;

    // The pen advances in unscaled font units and is scaled at each glyph, so
    // rounding never accumulates across the run.
    int pen = 0;

    for (char c : text)
    {
        if (c == ' ')
        {
            pen += mSheet->spaceAdvance;
            continue;
        }

        const BitmapFontSheet::Glyph * glyph = mSheet->find(c);
        if (!glyph) continue;

        glyph->image.draw(x + int(std::lround((pen + glyph->xoffset) * mScale)),
                          y + int(std::lround(glyph->yoffset * mScale)),
                          z, mScale, mScale, 0, alpha, color);

        pen += glyph->advance;
    }
}

void BitmapFont::drawWithShadow(const std::string & text, int x, int y, int z,
                                SDL_Color color,
                                int shadowX, int shadowY, SDL_Color shadowColor,
                                Uint8 alpha) const
{
    // Same geometry the old composited surfaces had: text at (x, y), shadow
    // offset behind it. Both passes share the atlas texture, so the shadow
    // costs no extra draw call -- only extra quads.
    //
    // Both passes also use the same z. That is safe *because* they share a
    // texture: DrawingQueue::sort() is a stable_sort keyed on (z, texture), so
    // ties keep insertion order and the shadow, queued first, stays behind.
    // Glyphs are baked white, so the shadow is just the same run tinted black
    // -- its alpha has to come from the alpha channel of shadowColor, since
    // colorMod only multiplies RGB.
    const Uint8 shadowAlpha = Uint8(alpha * shadowColor.a / 255);

    draw(text, x + shadowX, y + shadowY, z, shadowColor, shadowAlpha);
    draw(text, x, y, z, color, alpha);
}

std::vector<std::string> BitmapFont::wrapText(const std::string & text, int maxWidth) const
{
    std::vector<std::string> lines;
    if (!mSheet) return lines;

    std::string_view rest(text);

    while (true)
    {
        // Split off the next explicit paragraph, so "\n\n" keeps its blank line.
        const size_t breakPos = rest.find('\n');
        const std::string_view paragraph = rest.substr(0, breakPos);

        std::string line;

        size_t wordStart = 0;
        while (wordStart <= paragraph.size())
        {
            size_t wordEnd = paragraph.find(' ', wordStart);
            if (wordEnd == std::string_view::npos) wordEnd = paragraph.size();

            const std::string_view word = paragraph.substr(wordStart, wordEnd - wordStart);

            const std::string candidate = line.empty()
                ? std::string(word)
                : line + " " + std::string(word);

            // A word that cannot fit on its own still gets its own line
            // rather than being dropped or split mid-word.
            if (!line.empty() && getTextWidth(candidate) > maxWidth)
            {
                lines.push_back(line);
                line = std::string(word);
            }
            else
            {
                line = candidate;
            }

            if (wordEnd == paragraph.size()) break;
            wordStart = wordEnd + 1;
        }

        lines.push_back(line);

        if (breakPos == std::string_view::npos) break;
        rest = rest.substr(breakPos + 1);
    }

    return lines;
}
