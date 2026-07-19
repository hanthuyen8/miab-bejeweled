#include "Game.h"
#include "Assets.h"
#include "ZOrder.h"
#include "State.h"

#include "StateMainMenu.h"
#include "StateOptions.h"
#include "StateHowToPlay.h"
#include "StateGameTimetrial.h"
#include "StateGameEndless.h"

#include "MiabSDK.h"
#include "OptionsManager.h"


Game::Game ()
    : GoSDL::Window(800, 600, "Seajeweled")
{
    lDEBUG << Log::CON("Game");

    mMouseCursor.setWindow(this);
    mMouseCursor.setPath(Assets::HandCursor);

    // Must happen before the first changeState(): states build their fonts in
    // their constructors and read metrics straight out of this.
    if (!mFonts.load(this, Assets::AtlasImage, Assets::AtlasData, Assets::FontMetrics))
    {
        lDEBUG << "Game: font atlas failed to load, text will not render";
    }

    changeState("stateMainMenu");

    // Xin highscore hiện tại của user từ host page (MIAB), nếu có nhúng qua iframe. Async, không
    // block — nếu response chưa về kịp (hoặc không bao giờ về), ScoreTable vẫn dùng giá trị local
    // hiện có (mặc định 0) như trước.
    MiabSDK::RequestHighscore([](const std::string& mode, int score, int elapsedMs) {
        (void) elapsedMs;

        OptionsManager options;
        options.loadResources();

        if (mode == "endless") {
            options.setHighscoreEndless(score);
        } else if (mode == "timetrial") {
            options.setHighscoreTimetrial(score);
        }
    });
}

Game::~Game()
{
    lDEBUG << Log::DES("Game");
}

void Game::update ()
{
    if (mCurrentState)
        mCurrentState -> update();
}

void Game::draw ()
{
    if (getMouseActive()) {
        mMouseCursor.draw(getMouseX(), getMouseY(), Z::Cursor);
    }

    if (mCurrentState)
        mCurrentState -> draw();
}

void Game::buttonDown (SDL_Keycode button)
{
    if (handleCheatKey(button))
        return;

    if (mCurrentState)
        mCurrentState -> buttonDown(button);
}

#ifdef SEAJEWELED_CHEATS

bool Game::handleCheatKey (SDL_Keycode button)
{
    // Jumping straight to a screen, so testing one doesn't mean playing up to
    // it. The score table in particular is only reachable after a full
    // two-minute time trial run.
    //
    // Handled here rather than per state so the keys work from anywhere, and
    // consumed (returning true) so no state also acts on them. F9 is left
    // alone: the shell binds it to a Spector.js capture.
    switch (button)
    {
    case SDLK_F1: changeState("stateMainMenu");      return true;
    case SDLK_F2: changeState("stateGameEndless");   return true;
    case SDLK_F3: changeState("stateGameTimetrial"); return true;
    case SDLK_F4: changeState("stateHowtoplay");     return true;
    case SDLK_F5: changeState("stateOptions");       return true;

    case SDLK_F6:
    case SDLK_F7:
    {
        auto game = std::dynamic_pointer_cast<StateGame>(mCurrentState);
        if (!game) return true;

        if (button == SDLK_F6)
        {
            game->cheatAddScore(1337);
        }
        else
        {
            game->cheatShowScoreTable();
        }
        return true;
    }
    }

    return false;
}

#else

bool Game::handleCheatKey (SDL_Keycode)
{
    return false;
}

#endif

void Game::buttonUp (SDL_Keycode button)
{
    if (mCurrentState)
        mCurrentState -> buttonUp(button);
}

void Game::mouseButtonDown (Uint8 button)
{
    if (mCurrentState)
        mCurrentState -> mouseButtonDown(button);
}

void Game::mouseButtonUp (Uint8 button)
{
    if (mCurrentState)
        mCurrentState -> mouseButtonUp(button);
}

void Game::controllerButtonDown (Uint8 button)
{
    if (mCurrentState)
        mCurrentState -> controllerButtonDown(button);
}

void Game::changeState(string S)
{
    if(S == mCurrentStateString)
    {
        return;
    }
    else if(S == "stateMainMenu")
    {
        mCurrentState = std::make_shared<StateMainMenu>(this);
        mCurrentStateString = "stateMainMenu";
    }
    else if(S == "stateGameTimetrial")
    {
        mCurrentState = std::make_shared<StateGameTimetrial>(this);
        mCurrentStateString = "stateGameTimetrial";
    }
    else if(S == "stateGameEndless")
    {
        mCurrentState = std::make_shared<StateGameEndless>(this);
        mCurrentStateString = "stateGameEndless";
    }
    else if(S == "stateHowtoplay")
    {
        mCurrentState = std::make_shared<StateHowtoplay>(this);
        mCurrentStateString = "stateHowtoplay";
    }
    else if(S == "stateOptions")
    {
        mCurrentState = std::make_shared<StateOptions>(this);
        mCurrentStateString = "stateOptions";
    }
    else if(S == "stateQuit")
    {
        close();
    }
}

string Game::getCurrentState() {
    return mCurrentStateString;
}
