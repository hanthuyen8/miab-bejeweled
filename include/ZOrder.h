#ifndef _ZORDER_H_
#define _ZORDER_H_

// Central registry of draw depths (z-order). Larger z draws on top.
//
// The DrawingQueue orders draws by z only; two elements that overlap and need
// a fixed front-to-back order MUST use different z values (see the Step 1/2
// reports). Keeping every depth here makes the layer stack reviewable in one
// place and avoids magic numbers scattered across draw() calls.

namespace Z {

    // ---- In-game screen (StateGame / GameBoard / GameIndicators) ----
    // Layer stack, bottom -> top:
    constexpr int Board        = 0;   // board.png backdrop
    constexpr int UIPanel      = 2;   // score/time/button backgrounds, loading banner
    constexpr int Gem          = 3;   // gems on the board
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
