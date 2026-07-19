#ifndef GEM_SHINE_H
#define GEM_SHINE_H

#include "go_image.h"
#include "go_textureatlas.h"

/**
 * Pre-renders the "light streak sweeping across a gem" effect.
 *
 * A rectangular streak drawn straight over a gem would spill onto the board
 * behind it, since the gem art does not fill its 65px cell. Instead, at load
 * time every gem is composited with the streak at each step of the sweep into
 * a single sprite sheet, using a custom blend mode that multiplies the streak
 * by the gem's own alpha — so the highlight is clipped to the gem silhouette.
 *
 * The result costs nothing at draw time: gems are drawn exactly as before,
 * only pointed at a different frame of the sheet.
 */
class GemShine
{
public:

    /// Gem rows in the sheet. Order is arbitrary but must match the indices
    /// callers pass to attach()/setFrame().
    enum Gem
    {
        White = 0,
        Red,
        Purple,
        Orange,
        Green,
        Yellow,
        Blue,

        Count
    };

    /// Number of frames in one sweep. Frame 0 has the streak fully outside the
    /// cell, i.e. it is the untouched gem, which is what idle cells draw.
    static constexpr int kFrameCount = 24;

    /// Board cell size, and therefore the size of one sheet frame.
    static constexpr int kCellSize = 65;

    /// Tilt of the streak away from horizontal, in degrees. 90 is vertical.
    static constexpr double kStreakAngleDegrees = 80;

    /// Width of the streak image. It has to clear the cell on both sides so the
    /// sweep can start and end fully outside it, plus roughly another
    /// kCellSize * |lean| of horizontal room for the lean itself, or the streak
    /// comes out clipped at the top and bottom of the cell.
    static constexpr int kStreakWidth = 60;

    /// Distance the streak covers during one sweep, from fully off the left of
    /// a cell to fully off its right.
    static constexpr int kSweepTravel = kCellSize + kStreakWidth;

    /// The streak's tilt as a horizontal shift per pixel of height. Negative
    /// means the top edge sits further right, i.e. the light comes from above.
    ///
    /// GameBoard needs this to stagger the cells: the highlight is meant to be
    /// one straight line crossing the whole board, so a cell one row down has
    /// to be lit slightly later, by exactly the amount the line leans. Deriving
    /// both from this single value is what keeps the per-gem streak and the
    /// board-wide sweep at the same angle.
    static double streakLean();

    /// Composites the sheet from the gem sprites in `atlas`. Returns false if
    /// the sheet could not be created, in which case attach() is a no-op and
    /// callers keep their original (unanimated) images.
    bool bake(GoSDL::Window * window, const GoSDL::TextureAtlas & atlas);

    /// Points `image` at gem `gem` in the sheet, on the idle frame. Call once
    /// after bake(); afterwards only setFrame() is needed.
    void attach(GoSDL::Image & image, Gem gem) const;

    /// Selects the sweep frame of an already-attached image. Cheap: it only
    /// moves the source rectangle, so it can be called per cell, per frame.
    void setFrame(GoSDL::Image & image, Gem gem, int frame) const;

    bool isBaked() const { return mBaked; }

private:

    /// The composited sheet, kFrameCount columns by Count rows of cells. Held
    /// as an Image so it owns the texture and hands the same one to every gem.
    GoSDL::Image mSheet;

    bool mBaked = false;
};

#endif /* GEM_SHINE_H */
