#include "StateMainMenu.h"

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

// Number of bytes making up the first UTF-8 codepoint of `text`, so
// drop-cap splitting doesn't cut a multi-byte character in half.
static size_t firstUtf8CharLen(const std::string & text)
{
    if (text.empty()) return 0;

    unsigned char lead = text[0];
    if ((lead & 0x80) == 0x00) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}

StateMainMenu::StateMainMenu(Game * p) : State(p)
{

    mCurrentTransitionState = TransitionIn;

    // Init background image
    mImgBackground.setWindow(p);
    mImgBackground.setPath(Assets::MenuBackground);

    // Init logo image
    mImgLogo.setWindow(p);
    mImgLogo.setPath(Assets::MenuLogo);

    // Init menu highlight image
    mImgHighl.setWindow(p);
    mImgHighl.setPath(Assets::MenuHighlight);

    // Load the font
    mFont.setAll(p->getFonts(), Assets::Font::Menu, 26);

    // Larger font used for each entry's drop-cap first letter
    mFontDropCap.setAll(p->getFonts(), Assets::Font::Menu, 31);

    // Menu target states
    mMenuTargets = {"stateGameTimetrial", "stateGameEndless", "stateHowtoplay", "stateOptions", "stateQuit"};

    // Menu text items, split into a drop-cap first letter and the rest
    for (const std::string & label : {"TIMETRIAL MODE", "ENDLESS MODE", "HOW TO PLAY?", "OPTIONS", "EXIT"})
    {
        std::string translated = _(label.c_str());
        size_t dropCapLen = firstUtf8CharLen(translated);

        mMenuDropCapTexts.push_back(translated.substr(0, dropCapLen));
        mMenuRestTexts.push_back(translated.substr(dropCapLen));
    }

    // Jewel group animation
    mJewelAnimation.loadResources(p);

    mAnimationTotalSteps = 30;
    mAnimationLogoSteps = 30;
    mAnimationCurrentStep = 0;

    mMenuSelectedOption = 0;
    mMenuYStart = 350;
    mMenuYGap = 42;
    mMenuYEnd = mMenuYStart + (int) mMenuTargets.size() * mMenuYGap;

    mGame->getGameSounds()->loadResources();
}

void StateMainMenu::update(){

    if(mCurrentTransitionState == TransitionIn)
    {
        mAnimationCurrentStep ++;

        if(mAnimationCurrentStep == mAnimationTotalSteps)
        {
            mCurrentTransitionState = Active;
        }

    } else if(mCurrentTransitionState == Active){

    } else if(mCurrentTransitionState == TransitionOut){

    }

    if (mGame->getMouseActive()) {
        // Update menu highlighting according to mouse position
        int mY = (int) mGame -> getMouseY();

        if(mY >= mMenuYStart && mY < mMenuYEnd)
        {
            unsigned int hovered = (mY - mMenuYStart) / mMenuYGap;

            // Only the change fires the sound, not every frame spent on a row.
            // Keyboard navigation can't double up here: getMouseActive() goes
            // false on key press, and moveUp/moveDown play the sound themselves.
            if (hovered != mMenuSelectedOption)
            {
                mMenuSelectedOption = hovered;
                mGame->getGameSounds()->playSoundButtonHover();
            }
        }
    }
}

void StateMainMenu::draw(){

    // Draw the background
    mImgBackground.draw(0, 0, Z::Menu::Background);

    // Calculate the alpha value for the logo
    int logoAlfa = clamp( (int)(255 * (float)mAnimationCurrentStep / mAnimationLogoSteps),
                          0, 255);

    // Drawn 1:1 — the shipped art is already 628px wide, the size it appears at
    // (x=86 centres it on the 800px canvas). The full-resolution export lives in
    // assets/photoshop/mainMenuLogo.png; scaling it down at runtime meant
    // shipping ~7x the pixels and resampling them every frame.
    mImgLogo.draw(86, 86, Z::Menu::Logo, 1, 1, 0, logoAlfa);

    const SDL_Color menuTextColor = {229, 226, 233, 255}, menuShadowColor = {0, 0, 0, 128};

    // Loop to draw the menu items
    for(size_t i = 0, s = (int) mMenuTargets.size(); i < s; ++i)
    {
        const std::string & dropCapText = mMenuDropCapTexts[i];
        const std::string & restText = mMenuRestTexts[i];

        int dropCapWidth = mFontDropCap.getTextWidth(dropCapText);

        // Center the combined (drop-cap + rest) text horizontally, and
        // center the row vertically on the taller drop-cap glyph
        int totalWidth = dropCapWidth + mFont.getTextWidth(restText);
        int posX = std::round(800 / 2 - totalWidth / 2);
        int dropCapY = mMenuYStart + i * mMenuYGap + (mMenuYGap - mFontDropCap.getHeight()) / 2;

        // Shift the smaller rest-of-word text down so its baseline lines
        // up with the drop-cap's baseline
        int restY = dropCapY + (mFontDropCap.getAscent() - mFont.getAscent());

        // Draw the drop-cap first letter and the rest of the text
        mFontDropCap.drawWithShadow(dropCapText, posX, dropCapY, Z::Menu::Text,
            menuTextColor, 0, 2, menuShadowColor);
        mFont.drawWithShadow(restText, posX + dropCapWidth, restY, Z::Menu::Text,
            menuTextColor, 0, 2, menuShadowColor);
    }

    // Draw the menu highlighting, centered within the selected row
    int highlYOffset = (mMenuYGap - mImgHighl.getHeight()) / 2;
    mImgHighl.draw(266, mMenuYStart + highlYOffset + mMenuSelectedOption * mMenuYGap, Z::Menu::Highlight,
                   1, 1, 0, 255, {142, 123, 159, 255});

    // Draw the jewel animation
    mJewelAnimation.draw();
    //*/
}

void StateMainMenu::buttonDown(SDL_Keycode button)
{
    switch (button)
    {
        case SDLK_ESCAPE:
            mGame->close();
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

void StateMainMenu::controllerButtonDown(Uint8 button)
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

void StateMainMenu::mouseButtonDown(Uint8 button)
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

void StateMainMenu::moveUp() {
    mGame->getGameSounds()->playSoundButtonHover();

    if (mMenuSelectedOption == 0) {
        mMenuSelectedOption = mMenuTargets.size() - 1;
    } else {
        mMenuSelectedOption -= 1;
    }
}

void StateMainMenu::moveDown() {
    mGame->getGameSounds()->playSoundButtonHover();

    if (mMenuSelectedOption == mMenuTargets.size() - 1) {
        mMenuSelectedOption = 0;
    } else {
        mMenuSelectedOption += 1;
    }
}

void StateMainMenu::optionChosen()
{
    // Played before the state change, which tears this state down
    mGame->getGameSounds()->playSoundButtonClick();
    mGame -> changeState(mMenuTargets[mMenuSelectedOption]);
}

StateMainMenu::~StateMainMenu()
{
}
