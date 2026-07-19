#include "StateOptions.h"

#include "Assets.h"
#include "ZOrder.h"
#include "Game.h"
#include "log.h"
#include "inter.h"

#include <cmath>
#include <tuple>


template <typename T, typename R>

T clamp(T v, R bottom, R top)
{
    if(v > top) return top;
    if(v < bottom) return bottom;
    return v;
}

StateOptions::StateOptions(Game * p) : State(p)
{
    lDEBUG << Log::CON("StateOptions");

    mOptions.loadResources();

    // Init background image
    mImgBackground.setWindow(p);
    mImgBackground.setPath(Assets::MenuBackground);

    // Init menu highlight image
    mImgHighl.setWindow(p);
    mImgHighl.setPath(Assets::MenuHighlight);

    // Load the font
    mFont.setAll(p->getFonts(), Assets::Font::Menu, 30);

    // Menu target states
    mMenuOptions = {"setMusic", "setSound", "setFullscreen", "back"};

    updateButtonTexts();

    mMenuSelectedOption = 0;
    mMenuYStart = 150;
    mMenuYGap = 42;
    mMenuYEnd = mMenuYStart + (int) mMenuOptions.size() * mMenuYGap;

    mGame->getGameSounds()->loadResources();
}

void StateOptions::update(){

    if (mGame->getMouseActive()) {
        // Update menu highlighting according to mouse position
        int mY = (int) mGame -> getMouseY();

        if(mY >= mMenuYStart && mY < mMenuYEnd)
        {
            mMenuSelectedOption = (mY - mMenuYStart) / mMenuYGap;
        }
    }
}

void StateOptions::draw(){

    // Draw the background
    mImgBackground.draw(0, 0, Z::Menu::Background);

    // Loop to draw the menu items
    for(size_t i = 0, s = (int) mMenuOptions.size(); i < s; ++i)
    {
        // Calculate the horizontal and vertical positions
		int posX = std::round(800 / 2 - mFont.getTextWidth(mMenuTexts[i]) / 2),
            posY = mMenuYStart + i * mMenuYGap;

        // Draw the text and the shadow
        mFont.drawWithShadow(mMenuTexts[i], posX, posY, Z::Menu::Text,
            {255, 255, 255, 255}, 0, 2, {0, 0, 0, 128});
    }

    // Draw the menu highlighting
    mImgHighl.draw(266, mMenuYStart + 5 + mMenuSelectedOption * mMenuYGap, Z::Menu::Highlight);
}

void StateOptions::buttonDown(SDL_Keycode button)
{
    switch (button)
    {
        case SDLK_ESCAPE:
            mGame -> changeState("stateMainMenu");
            break;

        case SDLK_DOWN:
            moveDown();
            break;

        case SDLK_UP:
            moveUp();
            break;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            optionChosen();
            break;
    }
}

void StateOptions::controllerButtonDown(Uint8 button)
{
    switch (button)
    {
        case SDL_CONTROLLER_BUTTON_A:
            optionChosen();
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            moveDown();
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            moveUp();
            break;
    }
}

void StateOptions::mouseButtonDown(Uint8 button)
{
    if (button == SDL_BUTTON_LEFT)
    {
        // Get mouse vertical position
        int mY = mGame->getMouseY();

        if (mY >= mMenuYStart && mY <= mMenuYEnd)
        {
            optionChosen();
        }
    }
}

void StateOptions::moveUp() {
    mGame->getGameSounds()->playSoundSelect();

    if (mMenuSelectedOption == 0) {
        mMenuSelectedOption = mMenuOptions.size() - 1;
    } else {
        mMenuSelectedOption -= 1;
    }
}

void StateOptions::moveDown() {
    mGame->getGameSounds()->playSoundSelect();

    if (mMenuSelectedOption == mMenuOptions.size() - 1) {
        mMenuSelectedOption = 0;
    } else {
        mMenuSelectedOption += 1;
    }
}

void StateOptions::updateButtonTexts()
{
    vector<string> texts;

    // Option strings
    std::string musicText = _("Music: ");
    std::string soundText = _("Sound: ");
    std::string fullscreenText = _("Fullscreen: ");

    musicText += std::string(mOptions.getMusicEnabled() ? _("On") : _("Off"));
    soundText += std::string(mOptions.getSoundEnabled() ? _("On") : _("Off"));
    fullscreenText += std::string(mOptions.getFullscreenEnabled() ? _("On") : _("Off"));

    // Menu text items
    texts.push_back(musicText);
    texts.push_back(soundText);
    texts.push_back(fullscreenText);
    texts.push_back(_("Back"));

    mMenuTexts.swap(texts);
}

void StateOptions::optionChosen()
{
    string option = mMenuOptions[mMenuSelectedOption];
    if (option == "back") {
        mGame -> changeState("stateMainMenu");
    } else {
        if (option == "setMusic") {
            mOptions.setMusicEnabled(!mOptions.getMusicEnabled());
        } else if (option == "setSound") {
            mOptions.setSoundEnabled(!mOptions.getSoundEnabled());
            mGame->getGameSounds()->loadResources();
        } else if (option == "setFullscreen") {
            mOptions.setFullscreenEnabled(!mOptions.getFullscreenEnabled());
            mGame->setFullscreen(mOptions.getFullscreenEnabled());
        }
        updateButtonTexts();
    }
}

StateOptions::~StateOptions()
{
    lDEBUG << Log::DES("StateOptions");
}
