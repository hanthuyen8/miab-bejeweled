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

    // --- Bitmap fonts ---
    // Glyph metrics for the faces baked into the atlas. The source .ttf files
    // live in assets/fonts/ and are build-time inputs only: nothing under
    // media/ is a font any more, so no TTF is shipped or preloaded.
    constexpr const char * FontMetrics = "media/fonts.json";

    // Face names, as keyed in fonts.json (see texture-packer/generate_glyphs.py).
    namespace Font {
        constexpr const char * Menu   = "menu";    // Quicksand SemiBold — menus, buttons, headings
        constexpr const char * Normal = "normal";  // Miso Regular — body copy
        constexpr const char * Lcd    = "lcd";     // Quicksand Bold — score/time readouts
    }

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

        // Font glyphs also live in this atlas, but they are looked up through
        // media/fonts.json rather than by name here -- see BitmapFont.
    }
}

#endif /* _ASSETS_H_ */
