#include "StateLightningTest.h"

#include "Assets.h"
#include "ZOrder.h"
#include "Game.h"
#include "Util.h"

#include <cmath>
#include <algorithm>

namespace {
    constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;
    constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
}

// Tried nearest/point filtering on these textures (SDL_SetTextureScaleMode +
// SDL_ScaleModeNearest, overridable per-texture without touching the engine's
// global bilinear hint in go_window.cpp) to get a crisper pixel art look.
// Made it worse: these textures are continuous Gaussian gradients sampled at
// only 8x8, so nearest just turns each of their ~8 alpha steps into a big
// flat block -- more "barcode" than pixel art. Left on the engine's default
// bilinear until the source textures have enough resolution for nearest to
// make sense.

StateLightningTest::StateLightningTest(Game * p) : State(p), mFrameCounter(0), mSegmentCount(MinSegmentCount)
{
    mImgSegment.setWindowAndPath(mGame, Assets::Test::LightningSegment);
    mImgCap.setWindowAndPath(mGame, Assets::Test::LightningCap);
    mImgTip.setWindowAndPath(mGame, Assets::Test::LightningTip);
    mImgGlow.setWindowAndPath(mGame, Assets::Test::LightningGlow);
    mImgImpact.setWindowAndPath(mGame, Assets::Test::LightningImpact);

    mStart = {400.0, 300.0}; // center of the 800x600 window
    mEnd = {(double) mGame->getMouseX(), (double) mGame->getMouseY()};

    updateSegmentCount();
    reshuffleJitter();
    reshuffleBranches();
    rebuildChainPoints();
    rebuildBranchPoints();
}

void StateLightningTest::updateSegmentCount()
{
    double dx = mEnd.x - mStart.x;
    double dy = mEnd.y - mStart.y;
    double length = std::sqrt(dx * dx + dy * dy);

    int desired = (int) std::round(length / TargetSegmentLengthPx);
    desired = std::max(MinSegmentCount, std::min(MaxSegmentCount, desired));

    if (desired != mSegmentCount)
    {
        mSegmentCount = desired;
        mJitterOffsets.assign(mSegmentCount - 1, 0.0);
        reshuffleJitter();
    }
}

void StateLightningTest::reshuffleJitter()
{
    for (auto & offset : mJitterOffsets)
    {
        offset = getRandomFloat(-JitterPx, JitterPx);
    }
}

void StateLightningTest::reshuffleBranches()
{
    mBranchParams.clear();

    int branchCount = getRandomInt(BranchMinCount, BranchMaxCount);
    mBranchParams.reserve(branchCount);

    for (int i = 0; i < branchCount; ++i)
    {
        BranchParams branch;

        // Interior points only (not the very start/end of the main chain).
        branch.originIndex = getRandomInt(1, std::max(1, mSegmentCount - 1));
        branch.angleOffsetDeg = getRandomFloat(BranchMinAngleDeg, BranchMaxAngleDeg)
                               * (getRandomInt(0, 1) == 0 ? 1.0 : -1.0);
        branch.lengthFrac = getRandomFloat(BranchMinLengthFrac, BranchMaxLengthFrac);

        branch.jitterOffsets.assign(BranchSegmentCount - 1, 0.0);
        for (auto & offset : branch.jitterOffsets)
        {
            offset = getRandomFloat(-BranchJitterPx, BranchJitterPx);
        }

        mBranchParams.push_back(branch);
    }

    mBranchPoints.assign(mBranchParams.size(), {});
}

void StateLightningTest::rebuildChainPoints()
{
    double dx = mEnd.x - mStart.x;
    double dy = mEnd.y - mStart.y;
    double length = std::sqrt(dx * dx + dy * dy);

    // Degenerate case: cursor sits right on top of the origin, nothing to draw.
    if (length < 1.0)
    {
        mChainPoints.assign(mSegmentCount + 1, mStart);
        return;
    }

    double px = -dy / length, py = dx / length; // perpendicular to the main direction

    mChainPoints.clear();
    mChainPoints.reserve(mSegmentCount + 1);
    mChainPoints.push_back(mStart);

    for (int i = 1; i < mSegmentCount; ++i)
    {
        double t = (double) i / mSegmentCount;
        double baseX = mStart.x + dx * t;
        double baseY = mStart.y + dy * t;

        // Jitter tapers off near both ends so the bolt still reads as
        // pointing at its start/end rather than wandering off freely.
        double falloff = 1.0 - std::fabs(2.0 * t - 1.0) * 0.3;
        double offset = mJitterOffsets[i - 1] * falloff;

        mChainPoints.push_back({baseX + px * offset, baseY + py * offset});
    }

    mChainPoints.push_back(mEnd); // end point always locked exactly onto the cursor
}

void StateLightningTest::rebuildBranchPoints()
{
    double mainDx = mEnd.x - mStart.x;
    double mainDy = mEnd.y - mStart.y;
    double mainLength = std::sqrt(mainDx * mainDx + mainDy * mainDy);

    for (size_t b = 0; b < mBranchParams.size(); ++b)
    {
        vector<Point> & points = mBranchPoints[b];
        points.clear();

        if (mainLength < 1.0) continue;

        const BranchParams & params = mBranchParams[b];

        int originIndex = std::max(1, std::min(mSegmentCount - 1, params.originIndex));
        const Point & origin = mChainPoints[originIndex];

        double mainAngleRad = std::atan2(mainDy, mainDx);
        double branchAngleRad = mainAngleRad + params.angleOffsetDeg * kDegToRad;
        double branchLength = std::min(mainLength * params.lengthFrac, BranchMaxLengthPx);

        double bux = std::cos(branchAngleRad), buy = std::sin(branchAngleRad);
        double bpx = -buy, bpy = bux; // perpendicular to the branch direction

        points.reserve(BranchSegmentCount + 1);
        points.push_back(origin);

        for (int i = 1; i < BranchSegmentCount; ++i)
        {
            double t = (double) i / BranchSegmentCount;
            double baseX = origin.x + bux * branchLength * t;
            double baseY = origin.y + buy * branchLength * t;

            double falloff = 1.0 - std::fabs(2.0 * t - 1.0) * 0.3;
            double offset = params.jitterOffsets[i - 1] * falloff;

            points.push_back({baseX + bpx * offset, baseY + bpy * offset});
        }

        points.push_back({origin.x + bux * branchLength, origin.y + buy * branchLength});
    }
}

void StateLightningTest::drawBoltPiece(GoSDL::Image & sprite, const Point & p0, const Point & p1,
                                       double thicknessPx, SDL_Color tint, int z)
{
    double dx = p1.x - p0.x;
    double dy = p1.y - p0.y;
    double segLength = std::sqrt(dx * dx + dy * dy);

    if (segLength < 0.5) return; // avoid a degenerate zero-length sprite

    double ux = dx / segLength, uy = dy / segLength;
    double angleDeg = std::atan2(dy, dx) * kRadToDeg;

    // cap's left edge is the bolt's true origin, tip's right edge is its true
    // target -- neither needs to blend into anything, so they stay pinned.
    // Every other edge extends past the joint so its faded texture edge
    // overlaps the neighboring piece instead of just touching it.
    double overlapStart = (&sprite == &mImgCap) ? 0.0 : OverlapPx;
    double overlapEnd   = (&sprite == &mImgTip) ? 0.0 : OverlapPx;

    double shift = (overlapEnd - overlapStart) / 2.0;
    double midX = (p0.x + p1.x) / 2.0 + ux * shift;
    double midY = (p0.y + p1.y) / 2.0 + uy * shift;

    double destW = segLength + overlapStart + overlapEnd;
    double factorX = destW / sprite.getWidth();
    double factorY = thicknessPx / sprite.getHeight();
    double destH = sprite.getHeight() * factorY;

    sprite.draw((int) (midX - destW / 2.0), (int) (midY - destH / 2.0),
               z, factorX, factorY, (float) angleDeg, 255, tint);
}

void StateLightningTest::update()
{
    mEnd.x = mGame->getMouseX();
    mEnd.y = mGame->getMouseY();

    updateSegmentCount();

    if (++mFrameCounter >= ReshuffleIntervalFrames)
    {
        mFrameCounter = 0;
        reshuffleJitter();
        reshuffleBranches();
    }

    rebuildChainPoints();
    rebuildBranchPoints();
}

void StateLightningTest::draw()
{
    // Pale electric-blue tint applied at draw time -- textures are grayscale+alpha.
    SDL_Color tint = {160, 200, 255, 255};
    SDL_Color branchTint = {100, 125, 170, BranchAlpha}; // darker + dimmer than the main tint, so it recedes

    for (int i = 0; i < mSegmentCount; ++i)
    {
        GoSDL::Image * sprite = &mImgSegment;
        if (i == 0) sprite = &mImgCap;
        else if (i == mSegmentCount - 1) sprite = &mImgTip;

        drawBoltPiece(*sprite, mChainPoints[i], mChainPoints[i + 1], ThicknessPx, tint, Z::Test::Lightning);
    }

    for (const auto & points : mBranchPoints)
    {
        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            // No cap sprite here -- a branch doesn't originate from the source,
            // it forks off the main bolt's own body.
            GoSDL::Image * sprite = (i + 2 == points.size()) ? &mImgTip : &mImgSegment;

            drawBoltPiece(*sprite, points[i], points[i + 1],
                         ThicknessPx * BranchThicknessScale, branchTint, Z::Test::Lightning);
        }
    }

    // NOTE: the per-joint glow that used to sit here is disabled for now --
    // testing whether the segment/cap/tip edge-fade + overlap above (see
    // gen_lightning_sprites.py) is enough on its own to hide the seams
    // without a separate glow sprite competing for attention.

    // Impact burst at the target point (always the current cursor position).
    SDL_Color impactTint = {220, 235, 255, ImpactAlpha};
    double impactFactor = ImpactDisplaySizePx / mImgImpact.getWidth();
    double impactDest = mImgImpact.getWidth() * impactFactor;

    mImgImpact.draw((int) (mEnd.x - impactDest / 2.0), (int) (mEnd.y - impactDest / 2.0),
                    Z::Test::LightningImpact, impactFactor, impactFactor, 0, 255, impactTint);
}
