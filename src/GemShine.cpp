#include "GemShine.h"

#include "go_window.h"
#include "Assets.h"
#include "log.h"

#include <cmath>
#include <vector>

namespace {

    // Gem sprites in the same order as GemShine::Gem
    const char * const kGemSprites[GemShine::Count] = {
        Assets::Sprite::GemWhite,
        Assets::Sprite::GemRed,
        Assets::Sprite::GemPurple,
        Assets::Sprite::GemOrange,
        Assets::Sprite::GemGreen,
        Assets::Sprite::GemYellow,
        Assets::Sprite::GemBlue,
    };

    // The two bands making up the streak: a broad soft one and a narrow bright
    // companion trailing it, which is what sells the "polished glass" look.
    constexpr double kMainOffset = 0,  kMainSigma = 9,  kMainPeak = 0.62;
    constexpr double kEdgeOffset = 19, kEdgeSigma = 4,  kEdgePeak = 0.40;

    // Shorthands for the geometry now declared in the header
    constexpr int kStreakWidth = GemShine::kStreakWidth;
    const double kStreakLean = GemShine::streakLean();

    double bandIntensity (double u, double offset, double sigma, double peak)
    {
        double d = (u - offset) / sigma;
        return peak * std::exp(-d * d);
    }

    /// Builds the streak as a grayscale image with intensity in the RGB
    /// channels and alpha left at 255.
    ///
    /// Intensity has to live in RGB rather than alpha because of the blend mode
    /// used to composite it: the source colour is scaled by the *destination*
    /// alpha (the gem's silhouette), so the source's own alpha never reaches
    /// the colour equation.
    SDL_Texture * createStreakTexture (SDL_Renderer * renderer)
    {
        SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(
            0, kStreakWidth, GemShine::kCellSize, 32, SDL_PIXELFORMAT_RGBA32);

        if (surface == nullptr)
        {
            return nullptr;
        }

        SDL_LockSurface(surface);

        for (int y = 0; y < surface->h; ++y)
        {
            Uint8 * row = static_cast<Uint8 *>(surface->pixels) + y * surface->pitch;

            for (int x = 0; x < surface->w; ++x)
            {
                // Distance from the streak's axis, which leans as y grows
                double u = (x - kStreakWidth / 2.0)
                         - (y - GemShine::kCellSize / 2.0) * kStreakLean;

                double intensity = bandIntensity(u, kMainOffset, kMainSigma, kMainPeak)
                                 + bandIntensity(u, kEdgeOffset, kEdgeSigma, kEdgePeak);

                if (intensity > 1.0) intensity = 1.0;

                Uint8 level = static_cast<Uint8>(intensity * 255);

                Uint8 * pixel = row + x * 4;
                pixel[0] = level;
                pixel[1] = level;
                pixel[2] = level;
                pixel[3] = 255;
            }
        }

        SDL_UnlockSurface(surface);

        SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        return texture;
    }

    /// Adds the streak to whatever is already in the target, scaled by the
    /// target's alpha, and leaves that alpha untouched:
    ///
    ///     colour = streak * dstAlpha + colour
    ///     alpha  = alpha
    ///
    /// so the highlight is confined to the gem's silhouette and fades out
    /// exactly where the gem's edges do.
    SDL_BlendMode maskedAdditiveBlend()
    {
        return SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_DST_ALPHA, SDL_BLENDFACTOR_ONE,  SDL_BLENDOPERATION_ADD,
            SDL_BLENDFACTOR_ZERO,      SDL_BLENDFACTOR_ONE,  SDL_BLENDOPERATION_ADD);
    }
}

double GemShine::streakLean()
{
    // Negative so the top edge sits further right; see the header.
    return -1.0 / std::tan(kStreakAngleDegrees * M_PI / 180.0);
}

bool GemShine::bake(GoSDL::Window * window, const GoSDL::TextureAtlas & atlas)
{
    SDL_Renderer * renderer = window->getRenderer();

    const int sheetWidth  = kFrameCount * kCellSize;
    const int sheetHeight = Count * kCellSize;

    SDL_Texture * sheet = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET,
                                            sheetWidth, sheetHeight);

    if (sheet == nullptr)
    {
        lDEBUG << "GemShine: could not create the sheet: " << SDL_GetError();
        return false;
    }

    SDL_Texture * streak = createStreakTexture(renderer);

    if (streak == nullptr)
    {
        lDEBUG << "GemShine: could not create the streak: " << SDL_GetError();
        SDL_DestroyTexture(sheet);
        return false;
    }

    if (SDL_SetTextureBlendMode(streak, maskedAdditiveBlend()) != 0)
    {
        // Some backends reject custom blend modes. Plain additive still looks
        // reasonable here because the streak is clipped to the cell; it just
        // spills over the gem's edges onto the board behind it.
        lDEBUG << "GemShine: custom blend unsupported, falling back to additive";
        SDL_SetTextureBlendMode(streak, SDL_BLENDMODE_ADD);
    }

    SDL_Texture * previousTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, sheet);

    // Start from fully transparent, copying rather than blending so the gems'
    // own alpha lands in the sheet untouched — the whole effect depends on it.
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (int gem = 0; gem < Count; ++gem)
    {
        GoSDL::Image gemImage;

        if (!atlas.setImage(gemImage, kGemSprites[gem]))
        {
            lDEBUG << "GemShine: missing sprite " << kGemSprites[gem];
            continue;
        }

        SDL_Texture * gemTexture = gemImage.getTexture();
        SDL_Rect gemSrc;

        if (gemTexture == nullptr || !gemImage.getSrcRect(gemSrc))
        {
            lDEBUG << "GemShine: sprite not in an atlas: " << kGemSprites[gem];
            continue;
        }

        SDL_SetTextureBlendMode(gemTexture, SDL_BLENDMODE_NONE);

        for (int frame = 0; frame < kFrameCount; ++frame)
        {
            SDL_Rect cell = {frame * kCellSize, gem * kCellSize, kCellSize, kCellSize};

            // The streak is wider than the cell and mostly hangs outside it
            SDL_RenderSetClipRect(renderer, &cell);

            SDL_RenderCopy(renderer, gemTexture, &gemSrc, &cell);

            // Sweep the streak from just off the left edge to just off the
            // right one, so frame 0 is a clean gem and can be used while idle.
            double travel = double(frame) / (kFrameCount - 1);
            int streakX = cell.x - kStreakWidth
                        + static_cast<int>((kCellSize + kStreakWidth) * travel);

            SDL_Rect streakRect = {streakX, cell.y, kStreakWidth, kCellSize};
            SDL_RenderCopy(renderer, streak, nullptr, &streakRect);
        }

        // Leave the atlas as the rest of the game expects to find it
        SDL_SetTextureBlendMode(gemTexture, SDL_BLENDMODE_BLEND);
    }

    SDL_RenderSetClipRect(renderer, nullptr);
    SDL_SetRenderTarget(renderer, previousTarget);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_DestroyTexture(streak);

    SDL_SetTextureBlendMode(sheet, SDL_BLENDMODE_BLEND);

    mSheet.setWindow(window);
    mSheet.setTexture(sheet);

    mBaked = true;
    return true;
}

void GemShine::attach(GoSDL::Image & image, Gem gem) const
{
    if (!mBaked)
    {
        return;
    }

    // Copying shares the sheet's texture, which is what keeps every gem in a
    // single draw batch
    image = GoSDL::Image(mSheet);
    setFrame(image, gem, 0);
}

void GemShine::setFrame(GoSDL::Image & image, Gem gem, int frame) const
{
    if (!mBaked)
    {
        return;
    }

    if (frame < 0) frame = 0;
    if (frame >= kFrameCount) frame = kFrameCount - 1;

    SDL_Rect cell = {frame * kCellSize, gem * kCellSize, kCellSize, kCellSize};
    image.setSrcRect(cell);
}
