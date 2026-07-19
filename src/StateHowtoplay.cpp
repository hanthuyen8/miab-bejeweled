#include "StateHowToPlay.h"

#include "Assets.h"
#include "ZOrder.h"
#include "Game.h"
#include "inter.h"

StateHowtoplay::StateHowtoplay(Game * p) : State(p)
{
    lDEBUG << Log::CON("StateHowtoPlay");

    mImgBackground.setWindowAndPath(p, Assets::HowtoScreen);

    // Build the title text
    mFontTitle.setAll(p->getFonts(), Assets::Font::Menu, 48);
    mTitleText = _("");

    // Build the subtitle text
    mFontSubtitle.setAll(p->getFonts(), Assets::Font::Menu, 20);
    mSubtitleText = _("Click to go back");

    // Build the main text
    mFontBody.setAll(p->getFonts(), Assets::Font::Normal, 28);

    string bodyText = "";

    bodyText += _("The objective of the game is to swap one gem with an adjacent gem to form a horizontal or vertical chain of three or more gems.");
    bodyText += "\n\n";
    bodyText += _("Click the first gem and then click the gem you want to swap it with. If the movement is correct, they will swap and the chained gems will disappear.");
    bodyText += "\n\n";
    bodyText += _("Bonus points are given when more than three identical gems are formed. Sometimes chain reactions, called cascades, are triggered, where chains are formed by the falling gems. Cascades are awarded with bonus points.");

    // Wrapped once here; drawing it re-wrapped every frame would be wasteful.
    mBodyLines = mFontBody.wrapText(bodyText, kBodyWidth);
}

void StateHowtoplay::update() { }

void StateHowtoplay::draw()
{
    mImgBackground.draw(0, 0, Z::Howto::Background);

    const SDL_Color white = {255, 255, 255, 255}, shadow = {0, 0, 0, 128};

    mFontTitle.drawWithShadow(mTitleText,
        300 + 470 / 2 - mFontTitle.getTextWidth(mTitleText) / 2, 20,
        Z::Howto::Text, white, 1, 2, shadow);

    mFontSubtitle.drawWithShadow(mSubtitleText, 70, 545, Z::Howto::Text, white, 1, 2, shadow);

    int lineY = 110;
    for (const std::string & line : mBodyLines)
    {
        mFontBody.drawWithShadow(line, 310, lineY, Z::Howto::Text, white, 1, 2, shadow);
        lineY += mFontBody.getHeight();
    }
}

void StateHowtoplay::buttonDown(SDL_Keycode)
{
        mGame -> changeState("stateMainMenu");
}

void StateHowtoplay::controllerButtonDown(Uint8)
{
    mGame -> changeState("stateMainMenu");
}

void StateHowtoplay::mouseButtonDown(Uint8)
{
        mGame -> changeState("stateMainMenu");
}

StateHowtoplay::~StateHowtoplay()
{
    lDEBUG << Log::DES("StateHowtoPlay");
}
