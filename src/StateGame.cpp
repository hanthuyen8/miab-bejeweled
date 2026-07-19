#include "StateGame.h"

#include "BitmapFont.h"

#include "Assets.h"
#include "ZOrder.h"
#include "Game.h"
#include "inter.h"

#include <map>
#include <string>

StateGame::StateGame(Game * p) : State(p)
{

    setState(eInitial);

    // Initialise game indicator
    mGameIndicators.setGame(p, this);

    // Initialise game board
    mGameBoard.setGame(p, this);

    // Load the loading screen
    mFontLoading.setAll(mGame->getFonts(), Assets::Font::Menu, 64);
}

StateGame::~StateGame ()
{
}

void StateGame::draw()
{
    // On this state, show the loading screen and switch the state
    if (mState == eInitial)
    {
        mFontLoading.draw(_("Loading..."), 280, 250, Z::UIPanel);
        setState(eStartLoading);

        return;
    }

    // In all the other states, the full window is drawn
    mImgBoard.draw(0, 0, Z::Board);

    // Draw the indicators (buttons and labels)
    mGameIndicators.draw();

    // Draw the main game board
    mGameBoard.draw();
}

void StateGame::buttonDown(SDL_Keycode button)
{
    if (button == SDLK_ESCAPE)
    {
        exitToMenu();
    }

    else if (button == SDLK_h)
    {
        showHint();
    }
    
    else {
        mGameBoard.buttonDown(button);
    }
}

void StateGame::controllerButtonDown(Uint8 button)
{
    if (button == SDL_CONTROLLER_BUTTON_START) {
        exitToMenu();
    } else if (button == SDL_CONTROLLER_BUTTON_BACK) {
        resetGame();
    } else {
        mGameBoard.controllerButtonDown(button);
    }
}

void StateGame::mouseButtonDown(Uint8 button)
{
    // Left mouse button was pressed
    if (button == SDL_BUTTON_LEFT)
    {
        mMousePressed = true;

        // Get click location
        int mouseX = mGame->getMouseX();
        int mouseY = mGame->getMouseY();

        // Inform the UI
        mGameIndicators.click(mouseX, mouseY);

        // Inform the board
        mGameBoard.mouseButtonDown(mouseX, mouseY);
    }
}

void StateGame::mouseButtonUp(Uint8 button)
{
    // Left mouse button was released
    if (button == SDL_BUTTON_LEFT)
    {
        mMousePressed = false;

        // Get click location
        int mouseX = mGame->getMouseX();
        int mouseY = mGame->getMouseY();

        // Inform the board
        mGameBoard.mouseButtonUp(mouseX, mouseY);
    }
}

void StateGame::setState (tState state)
{
    // static std::map<tState, std::string> stateToString = {
    //     { eInitial, "eInitial"},
    //     { eStartLoading, "eStartLoading"},
    //     { eLoading, "eLoading"},
    //     { eLaunched, "eLaunched"},
    //     { eNewGemsFalling, "eNewGemsFalling"},
    //     { eOldGemsFalling, "eOldGemsFalling"},
    //     { eWaiting, "eWaiting"},
    //     { eGemSelected, "eGemSelected"},
    //     { eGemsSwitching, "eGemsSwitching"},
    //     { eGemsDisappearing, "eGemsDisappearing"},
    //     { eFirstFlip, "eFirstFlip"},
    //     { eInicialGemas, "eInicialGemas"},
    //     { eEspera, "eEspera"},
    //     { eGemaMarcada, "eGemaMarcada"},
    //     { eGemasCambiando, "eGemasCambiando"},
    //     { eGemasDesapareciendo, "eGemasDesapareciendo"},
    //     { eGemasNuevasCayendo, "eGemasNuevasCayendo"},
    //     { eDesapareceBoard, "eDesapareceBoard"},
    //     { eTimeFinished, "eTimeFinished"},
    //     { eShowingScoreTable, "eShowingScoreTable"}
    // };

    // lDEBUG << "New state: " << stateToString[state];
    mState = state;
}

// ----------------------------------------------------------------------------

void StateGame::loadResources()
{
    // Load the background image
    mImgBoard.setWindowAndPath(mGame, Assets::Board);

    mGameIndicators.loadResources();
    mGameBoard.loadResources();
}

void StateGame::resetGame()
{
    mGameIndicators.setScore(0);
    resetTime();
    mGameBoard.resetGame();
}

void StateGame::exitToMenu()
{
    mGameBoard.submitScoreOnQuit(getScore(), getElapsedMs());
    mGame -> changeState("stateMainMenu");
}

void StateGame::resetTime()
{
    // Default time is 2 minutes
    mTimeStart = SDL_GetTicks() + 2 * 60 * 1000;

    // Both modes call this when a run starts, so it is also the one place that
    // knows when "now" began — timetrial used to re-derive it by subtracting
    // the 2 minutes above back off mTimeStart, which silently produced a wrong
    // elapsed time the moment that duration was changed in only one of the two.
    mPlayStartTicks = SDL_GetTicks();
}

int StateGame::getElapsedMs() const
{
    return (int)(SDL_GetTicks() - mPlayStartTicks);
}

void StateGame::showHint()
{
    mGameBoard.showHint();
}

void StateGame::increaseScore (int amount)
{
    mGameIndicators.increaseScore(amount);
}

int StateGame::getScore() {
    return mGameIndicators.getScore();
}

#ifdef SEAJEWELED_CHEATS

void StateGame::cheatAddScore(int amount) {
    mGameIndicators.increaseScore(amount);
}

void StateGame::cheatShowScoreTable() {
    mGameBoard.showScoreTableForTesting(mGameIndicators.getScore());
}

#endif
