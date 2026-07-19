#ifndef _ZORDER_H_
#define _ZORDER_H_

// Central registry of draw depths (z-order). Larger z draws on top.
//
// The DrawingQueue orders draws by z only; two elements that overlap and need
// a fixed front-to-back order MUST use different z values (see the Step 1/2
// reports). Keeping every depth here makes the layer stack reviewable in one
// place and avoids magic numbers scattered across draw() calls.
//
// Z ALSO DECIDES BATCHING. The queue sorts by (z, texture), so draws sharing a
// texture only collapse into one GPU batch if they are adjacent after sorting.
// Anything drawn from a DIFFERENT texture that lands in the middle of a z-range
// splits that range into two batches.
//
// In the game screen only three textures are in play: board.png, the GemShine
// sheet and the shared atlas (everything else). Note the gem art IS packed in
// the atlas, but it is not drawn from there: GemShine reads it out of the atlas
// once and composites each gem with its 24 sweep frames into a render target of
// its own, then repoints the gem images at that sheet (see GemShine::attach).
// The atlas is only the bake input. So the gems must sit at a z either fully
// below or fully above the atlas layers, never between them -- which is why
// Gem is 1, directly above the backdrop, rather than up among the UI. The
// board and the left-hand UI panel never overlap, so nothing is gained by
// interleaving them, and doing so cost a whole extra draw call.

namespace Z {

    // ---- In-game screen (StateGame / GameBoard / GameIndicators) ----
    // Layer stack, bottom -> top:
    constexpr int Board        = 0;   // board.png backdrop
    constexpr int Gem          = 1;   // gems on the board
    constexpr int UIPanel      = 2;   // score/time/button backgrounds, loading banner
    constexpr int UIText       = 3;   // score/time numbers + headers (above UIPanel)
    constexpr int ScoreTable   = 3;   // end-of-game score table
    constexpr int Selector     = 4;   // square selector
    constexpr int Particle     = 7;   // combo particles
    constexpr int Hint         = 10;  // hint selector pulse
    constexpr int FloatingScore = 80; // floating "+N" score popups

    // Button internal sub-layers, added to the button's base z (= UIPanel):
    namespace Button {
        constexpr int Icon    = 1;    // z + 1
        constexpr int Caption = 2;    // z + 2
    }

    // ---- Main menu / Options screens ----
    namespace Menu {
        constexpr int Background = 1;
        constexpr int Logo       = 2;
        constexpr int Highlight  = 2;
        constexpr int Jewel      = 2;   // falling gem animation
        constexpr int Text       = 3;   // menu option labels
    }

    // ---- How-to-play screen ----
    namespace Howto {
        constexpr int Background = 0;
        constexpr int Text       = 1;   // title / subtitle / body
    }

    // ---- Global overlay ----
    constexpr int Cursor = 999;         // mouse cursor, always on top
}

#endif /* _ZORDER_H_ */
