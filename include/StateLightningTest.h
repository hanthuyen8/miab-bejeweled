#ifndef _STATELIGHTNINGTEST_H_
#define _STATELIGHTNINGTEST_H_

#include "State.h"
#include "go_image.h"

#include <vector>

using namespace std;

class Game;

/**
 * @class StateLightningTest
 *
 * @brief Throwaway test scene for the segment-chain lightning effect.
 *
 * Fires a lightning bolt from the center of the screen to the current mouse
 * cursor position -- cap/segment/tip sprites for the chain body, a soft glow
 * over every joint (covers the seams between segments) and a bigger impact
 * burst at the target point. No gameplay/GameBoard involved. See
 * docs/lightning-effect/lightning-effect.md for the algorithm this
 * implements (perpendicular jitter, end point locked to target).
 *
 * Reached by booting straight into it (see Game::Game(), gated behind the
 * SEAJEWELED_TEST_SCENE compile define) rather than through the main menu.
 */
class StateLightningTest : public State {
public:
    StateLightningTest(Game * p);

    void update();
    void draw();

private:
    struct Point { double x, y; };

    /// A short offshoot growing out of a random interior point of the main
    /// chain -- randomized once per reshuffle, rebuilt (repositioned) every
    /// frame so it tracks the main chain as it moves.
    struct BranchParams {
        int originIndex;             // index into mChainPoints this branch grows from
        double angleOffsetDeg;       // signed deviation from the main bolt's direction
        double lengthFrac;           // branch length as a fraction of the main bolt's length
        vector<double> jitterOffsets;
    };

    /// Recomputes mSegmentCount from the current start-end distance. When it
    /// changes, mJitterOffsets is resized and reshuffled to match (old offsets
    /// don't line up with a different segment count).
    void updateSegmentCount();

    /// Rebuilds mChainPoints from mStart/mEnd and the current mJitterOffsets.
    /// Called every frame so the bolt tracks the cursor smoothly even on
    /// frames where the jitter itself isn't refreshed.
    void rebuildChainPoints();

    /// Draws new random perpendicular offsets for the interior chain points.
    void reshuffleJitter();

    /// Rerolls how many branches exist and their random parameters.
    void reshuffleBranches();

    /// Rebuilds mBranchPoints from mChainPoints and mBranchParams. Called
    /// every frame, same reasoning as rebuildChainPoints().
    void rebuildBranchPoints();

    /// Draws one piece (cap/segment/tip) of a chain from p0 to p1. Extends the
    /// drawn length past whichever end(s) aren't the bolt's true start/end, so
    /// the sprite's faded edges (see gen_lightning_sprites.py) physically
    /// overlap the neighboring piece instead of just touching it -- `sprite`
    /// identity (compared against mImgCap/mImgTip) decides which end(s), if
    /// any, stay pinned exactly at p0/p1.
    void drawBoltPiece(GoSDL::Image & sprite, const Point & p0, const Point & p1,
                       double thicknessPx, SDL_Color tint, int z);

    GoSDL::Image mImgSegment;
    GoSDL::Image mImgCap;
    GoSDL::Image mImgTip;
    GoSDL::Image mImgGlow;
    GoSDL::Image mImgImpact;

    Point mStart;
    Point mEnd;

    vector<double> mJitterOffsets;
    vector<Point> mChainPoints;

    vector<BranchParams> mBranchParams;
    vector<vector<Point>> mBranchPoints;

    int mFrameCounter;
    int mSegmentCount;

    // Segment count scales with distance (~1 segment per TargetSegmentLengthPx)
    // instead of being fixed, so a long bolt doesn't stretch a handful of
    // segments thin -- clamped so it never goes below/above a sane count.
    static constexpr int MinSegmentCount = 4;
    static constexpr int MaxSegmentCount = 18;
    static constexpr double TargetSegmentLengthPx = 45.0;

    static constexpr int ReshuffleIntervalFrames = 4; // ~65ms at 60fps -- the "electric flicker" cadence
    static constexpr double JitterPx = 16.0;
    static constexpr double ThicknessPx = 10.0;

    // How far each piece's drawn length extends past a joint into its
    // neighbor, so the two faded edges overlap and blend instead of abutting.
    static constexpr double OverlapPx = 10.0;

    // Secondary "child" bolts: shorter, dimmer, thinner, forking off the main
    // chain at a random angle -- mimics real lightning's branching.
    static constexpr int BranchMinCount = 1;
    static constexpr int BranchMaxCount = 2;
    static constexpr int BranchSegmentCount = 3;
    static constexpr double BranchMinAngleDeg = 25.0;
    static constexpr double BranchMaxAngleDeg = 65.0;
    static constexpr double BranchMinLengthFrac = 0.15;
    static constexpr double BranchMaxLengthFrac = 0.35;
    // Absolute cap on branch length regardless of the main bolt's length, so a
    // long-distance bolt (many segments) doesn't grow branches long enough to
    // compete with the main chain -- they stay short accents at any distance.
    static constexpr double BranchMaxLengthPx = 70.0;
    static constexpr double BranchJitterPx = JitterPx * 0.6;
    static constexpr double BranchThicknessScale = 0.5;
    static constexpr Uint8 BranchAlpha = 90;

    // Glow drawn over every joint of the main chain, to hide the seams
    // between rotated segments. Kept only slightly bigger than the beam
    // itself and fairly transparent -- it's meant to soften a seam, not read
    // as a chain of beads strung along the bolt.
    static constexpr double GlowDisplaySizePx = ThicknessPx * 1.3;
    static constexpr Uint8 GlowAlpha = 110;

    // Bigger flash drawn once, at the point the bolt is aimed at.
    static constexpr double ImpactDisplaySizePx = 50.0;
    static constexpr Uint8 ImpactAlpha = 230;
};

#endif /* _STATELIGHTNINGTEST_H_ */
