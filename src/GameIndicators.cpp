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
    // Load the font for the timer
    mFontTime.setAll(mGame, Assets::FontLcd, 62);

    // Load the font for the scoreboard
    mFontScore.setAll(mGame, Assets::FontLcd, 33);

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
    std::string mHintButtonText = _("Show hint");
    std::string mResetButtonText = _("Reset game");
    std::string mExitButtonText = _("Exit");

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
    // Regenerate the texture if the score has changed
    if (mScore != mScorePrevious)
    {
        mImgScore = mFontScore.renderText(std::to_string(mScore), {78, 193, 190, 255});
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

        std::string txtTime = std::to_string(minutes) +
            (seconds < 10 ? ":0" : ":") +
            std::to_string(seconds);

        mImgTime = mFontTime.renderText(txtTime, {78, 193, 190, 255});

        mRemainingTimePrevious = mRemainingTime;
    }
}

void GameIndicators::draw()
{
    // Vertical initial position for the buttons
    int vertButStart = 407;

    // Draw the buttons
    mHintButton.draw(17, vertButStart, Z::UIPanel);
    mResetButton.draw(17, vertButStart + 47, Z::UIPanel);
    mExitButton.draw(17, 538, Z::UIPanel);

    // Draw the score. The number sits on top of its background, so it must
    // use a higher z than the background (same convention as BaseButton) —
    // the drawing queue only guarantees ordering by z, not by insertion order.
    mImgScoreBackground.draw(17, 124, Z::UIPanel);
    mImgScoreHeader.draw(17 + mImgScoreBackground.getWidth() / 2 - mImgScoreHeader.getWidth() / 2, 84, Z::UIText);
    mImgScore.draw(197 - mImgScore.getWidth(), 127, Z::UIText);

    // Draw the time
    if (mTimeEnabled) {
        mImgTimeBackground.draw(17, 230, Z::UIPanel);
        mImgTimeHeader . draw(17 + mImgTimeBackground.getWidth() / 2 - mImgTimeHeader.getWidth() / 2, 190, Z::UIText);
        mImgTime.draw(190 - mImgTime.getWidth(), 232, Z::UIText);
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