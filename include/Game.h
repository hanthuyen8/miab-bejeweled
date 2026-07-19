#ifndef _GAME_H_
#define _GAME_H_

#include <string>
#include <memory>

#include "BitmapFont.h"
#include "GameSounds.h"
#include "go_window.h"
#include "go_image.h"


class State;

class Game : public GoSDL::Window
{

public:
    Game ();
    ~Game();

    void update();

    void draw();

    void buttonDown(SDL_Keycode button);
    void buttonUp(SDL_Keycode button);
    void mouseButtonDown(Uint8 button);
    void mouseButtonUp(Uint8 button);
    void controllerButtonDown(Uint8 button);

    void changeState(std::string S);

    std::string getCurrentState();

    GameSounds *getGameSounds() {
        return &mGameSounds;
    }

    /// Glyph atlas shared by every state. Loaded once in the constructor and
    /// valid for the lifetime of the Game, so states can hold BitmapFonts
    /// pointing into it without owning anything.
    const BitmapFontAtlas *getFonts() const {
        return &mFonts;
    }

private:

    /// Debug hotkeys for reaching a screen without playing up to it. Returns
    /// true when the key was a cheat and should not reach the current state.
    /// Compiled out unless SEAJEWELED_CHEATS is defined (see CMakeLists.txt).
    bool handleCheatKey(SDL_Keycode button);

    std::shared_ptr<State> mCurrentState = nullptr;
    std::string mCurrentStateString;

    GoSDL::Image mMouseCursor;

    /// Sounds controller
    GameSounds mGameSounds;

    /// Every baked glyph, for all faces and sizes
    BitmapFontAtlas mFonts;
};

#endif /* _GAME_H_ */
