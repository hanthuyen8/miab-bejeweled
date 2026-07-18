#include "GameIndicators.h"
#include "Game.h"
#include "inter.h"

#include "StateGame.h"
#include "go_textureatlas.h"
#include "Assets.h"
#include "ZOrder.h"

GameIndicators::GameIndicators() :
    mGame (NULL),
    mStateGame (NULL)
{

}

void GameIndicators::setGame (Game * g, StateGame * sg)
{
    mGame = g;
    mStateGame = sg;
}

void GameIndicators::loadResources()
{
    // Score/time digits are drawn from the atlas (bitmap font)
    mNumbers.loadResources(mGame);

    // Font to render some headers
    GoSDL::Font tempHeaderFont;
    tempHeaderFont.setAll(mGame, Assets::FontNormal, 37);

    mImgScoreHeader = tempHeaderFont.renderTextWithShadow(_("score"), {160, 169, 255, 255}, 1, 1, {0, 0, 0, 128});

    mImgTimeHeader = tempHeaderFont.renderTextWithShadow(_("time left"), {160, 169, 255, 255}, 1, 1, {0, 0, 0, 128});

    // Background images come from the shared atlas so they batch together
    GoSDL::TextureAtlas atlas;
    atlas.load(mGame, Assets::AtlasImage, Assets::AtlasData);

    // Load the background image for the time
    atlas.setImage(mImgTimeBackground, Assets::Sprite::TimeBackground);

    // Load the background image for the scoreboard
    atlas.setImage(mImgScoreBackground, Assets::Sprite::ScoreBackground);

    // Buttons
    std::string mHintButtonText = _("HINT");
    std::string mResetButtonText = _("RESET");
    std::string mExitButtonText = _("EXIT");

    #ifdef __vita__
        mHintButtonText += std::string(" (/\\)");
        mResetButtonText += std::string(" (SEL)");
        mExitButtonText += std::string(" (START)");
    #endif

    mHintButton.set(mGame,  mHintButtonText.c_str(), Assets::Sprite::IconHint);
    mResetButton.set(mGame, mResetButtonText.c_str(), Assets::Sprite::IconRestart);
    mExitButton.set(mGame, mExitButtonText.c_str(), Assets::Sprite::IconExit);

    // Music
    options.loadResources();

    if (options.getMusicEnabled()) {
        sfxSong.setSample(Assets::Music);
        sfxSong.play();
    }
}

int GameIndicators::getScore()
{
    return mScore;
}

void GameIndicators::setScore (int score)
{
    mScore = score;

    regenerateScoreTexture();
}

void GameIndicators::increaseScore (int amount)
{
    mScore += amount;

    regenerateScoreTexture();
}

void GameIndicators::regenerateScoreTexture()
{
    // Update the score string if it has changed
    if (mScore != mScorePrevious)
    {
        mScoreText = std::to_string(mScore);
        mScorePrevious = mScore;
    }
}

void GameIndicators::updateTime (double time)
{
    mRemainingTime = time;

    // Only recreate the tiem string if it's changed
    if (mRemainingTime >= 0 && mRemainingTime != mRemainingTimePrevious)
    {
        int minutes = int(mRemainingTime / 60);
        int seconds = int(mRemainingTime - minutes * 60);

        mTimeText = std::to_string(minutes) +
            (seconds < 10 ? ":0" : ":") +
            std::to_string(seconds);

        mRemainingTimePrevious = mRemainingTime;
    }
}

void GameIndicators::draw()
{
    // Vertical initial position for the buttons, and the gap kept between
    // each stacked button (based on the current button art's height)
    int vertButStart = 356;
    int buttonSpacing = mHintButton.getHeight() + 5;

    // Draw the buttons
    int horizButPos = 45;
    mHintButton.draw(horizButPos, vertButStart, Z::UIPanel);
    mResetButton.draw(horizButPos, vertButStart + buttonSpacing, Z::UIPanel);
    mExitButton.draw(horizButPos, vertButStart + 2 * buttonSpacing, Z::UIPanel);

    // LCD colour shared by the score and time digits
    const SDL_Color lcdColor = {78, 193, 190, 255};
    const int scoreFontSize = 33;
    const int timeFontSize = 62;

    // Draw the score. The number sits on top of its background, so it must
    // use a higher z than the background (same convention as BaseButton) —
    // the drawing queue only guarantees ordering by z, not by insertion order.
    // The number is right-aligned to x=197.
    mImgScoreBackground.draw(17, 124, Z::UIPanel);
    mImgScoreHeader.draw(17 + mImgScoreBackground.getWidth() / 2 - mImgScoreHeader.getWidth() / 2, 84, Z::UIText);
    mNumbers.draw(mScoreText, 197 - mNumbers.width(mScoreText, scoreFontSize), 127, Z::UIText, scoreFontSize, lcdColor);

    // Draw the time
    if (mTimeEnabled) {
        mImgTimeBackground.draw(17, 230, Z::UIPanel);
        mImgTimeHeader . draw(17 + mImgTimeBackground.getWidth() / 2 - mImgTimeHeader.getWidth() / 2, 190, Z::UIText);
        mNumbers.draw(mTimeText, 190 - mNumbers.width(mTimeText, timeFontSize), 232, Z::UIText, timeFontSize, lcdColor);
    }
}

void GameIndicators::click(int mouseX, int mouseY)
{
    // Exit button was clicked
    if (mExitButton.clicked(mouseX, mouseY))
    {
        mGame -> changeState("stateMainMenu");
    }

    // Hint button was clicked
    else if (mHintButton.clicked(mouseX, mouseY))
    {
        mStateGame -> showHint();
    }

    // Reset button was clicked
    else if (mResetButton.clicked(mouseX, mouseY))
    {
        mStateGame -> resetGame();
    }
}

void GameIndicators::disableTime() {
    mTimeEnabled = false;
}

void GameIndicators::enableTime() {
    mTimeEnabled = true;
}