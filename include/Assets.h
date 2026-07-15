#ifndef _ASSETS_H_
#define _ASSETS_H_

// Central registry of every runtime asset path and every atlas sprite name.
// Use these constants instead of hard-coded "media/..." strings so usages are
// greppable and paths live in one place.
//
// Layout on disk (see docs/repo-structure.md):
//   media/            runtime assets, shipped / preloaded
//   assets/sprites/   source art packed into media/atlas.png (NOT loaded at runtime)

namespace Assets {

    // --- Texture atlas (runtime) ---
    constexpr const char * AtlasImage = "media/atlas.png";
    constexpr const char * AtlasData  = "media/atlas.json";

    // --- Standalone runtime images ---
    constexpr const char * Board       = "media/images/board.png";
    constexpr const char * HowtoScreen = "media/images/howtoScreen.png";
    constexpr const char * HandCursor  = "media/images/handCursor.png";

    // --- Main menu images ---
    constexpr const char * MenuBackground = "media/menu/mainMenuBackground.png";
    constexpr const char * MenuLogo       = "media/menu/mainMenuLogo.png";
    constexpr const char * MenuHighlight  = "media/menu/menuHighlight.png";

    // --- Fonts ---
    constexpr const char * FontLcd    = "media/fonts/fuentelcd.ttf";
    constexpr const char * FontNormal = "media/fonts/fuenteNormal.ttf";
    constexpr const char * FontMenu   = "media/fonts/fuenteMenu.ttf";

    // --- Sounds ---
    constexpr const char * SfxMatch1 = "media/sounds/match1.ogg";
    constexpr const char * SfxMatch2 = "media/sounds/match2.ogg";
    constexpr const char * SfxMatch3 = "media/sounds/match3.ogg";
    constexpr const char * SfxSelect = "media/sounds/select.ogg";
    constexpr const char * SfxFall   = "media/sounds/fall.ogg";
    constexpr const char * Music     = "media/sounds/music.ogg";

    // --- Atlas sprite names (frame keys inside atlas.json) ---
    namespace Sprite {
        constexpr const char * GemWhite  = "gemWhite.png";
        constexpr const char * GemRed    = "gemRed.png";
        constexpr const char * GemPurple = "gemPurple.png";
        constexpr const char * GemOrange = "gemOrange.png";
        constexpr const char * GemGreen  = "gemGreen.png";
        constexpr const char * GemYellow = "gemYellow.png";
        constexpr const char * GemBlue   = "gemBlue.png";

        constexpr const char * Selector  = "selector.png";
        constexpr const char * Particle1 = "partc1.png";
        constexpr const char * Particle2 = "partc2.png";

        constexpr const char * ButtonBackground = "buttonBackground.png";
        constexpr const char * ScoreBackground  = "scoreBackground.png";
        constexpr const char * TimeBackground   = "timeBackground.png";

        constexpr const char * IconHint    = "iconHint.png";
        constexpr const char * IconRestart = "iconRestart.png";
        constexpr const char * IconExit    = "iconExit.png";

        // LCD bitmap-font glyphs (fuentelcd, baked white @72px) for score/time
        constexpr const char * Digit0 = "digit0.png";
        constexpr const char * Digit1 = "digit1.png";
        constexpr const char * Digit2 = "digit2.png";
        constexpr const char * Digit3 = "digit3.png";
        constexpr const char * Digit4 = "digit4.png";
        constexpr const char * Digit5 = "digit5.png";
        constexpr const char * Digit6 = "digit6.png";
        constexpr const char * Digit7 = "digit7.png";
        constexpr const char * Digit8 = "digit8.png";
        constexpr const char * Digit9 = "digit9.png";
        constexpr const char * Colon  = "colon.png";
    }
}

#endif /* _ASSETS_H_ */
