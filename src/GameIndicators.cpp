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
    tempHeaderFont.setAll(mGame, Assets::FontButton, 20);

    mImgScoreHeader = tempHeaderFont.renderTextWithShadow(_("SCORE"), {229, 216, 196, 255}, 1, 1, {0, 0, 0, 128});

    mImgTimeHeader = tempHeaderFont.renderTextWithShadow(_("TIME LEFT"), {229, 216, 196, 255}, 1, 1, {0, 0, 0, 128});

    // Background images come from the shared atlas so they batch together
    GoSDL::TextureAtlas atlas;
    atlas.load(mGame, Assets::AtlasImage, Assets::AtlasData);

    // Load the background image for the time
    atlas.setImage(mImgTimeBackground, Assets::Sprite::TimeBackground);

    // Load the background image for the scoreboard
    atlas.setImage(mImgScoreBackground, Assets::Sprite::ScoreBackground);

    // Buttons
    mHintButton.set(mGame,  _("HINT"), Assets::Sprite::IconHint);
    mResetButton.set(mGame, _("RESET"), Assets::Sprite::IconRestart);
    mExitButton.set(mGame, _("EXIT"), Assets::Sprite::IconExit);

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
    const SDL_Color lcdColor = {229, 216, 196, 255};
    const int scoreFontSize = 30;
    const int timeFontSize = 30;

    // Score label, score background, time label and time background are
    // stacked top to bottom starting at y=170, sized off the actual image
    // heights so the layout self-adjusts if any of these assets change.
    int panelX = 45;
    int panelWidth = 150;
    int cursorY = 150;
    int groupGap = 2;

    // Draw the score. The number sits on top of its background, so it must
    // use a higher z than the background (same convention as BaseButton) —
    // the drawing queue only guarantees ordering by z, not by insertion order.
    // The label is centered on the whole panel width, not just the background.
    mImgScoreHeader.draw(panelX + panelWidth / 2 - mImgScoreHeader.getWidth() / 2, cursorY, Z::UIText);
    cursorY += mImgScoreHeader.getHeight() + groupGap;

    int scoreBgY = cursorY;
    mImgScoreBackground.draw(panelX, scoreBgY, Z::UIPanel);
    mNumbers.draw(mScoreText,
        panelX + (mImgScoreBackground.getWidth() - mNumbers.width(mScoreText, scoreFontSize)) / 2,
        scoreBgY + (mImgScoreBackground.getHeight() - mNumbers.height(scoreFontSize)) / 2,
        Z::UIText, scoreFontSize, lcdColor);
    cursorY += mImgScoreBackground.getHeight() + groupGap;

    // Draw the time
    if (mTimeEnabled) {
        mImgTimeHeader.draw(panelX + panelWidth / 2 - mImgTimeHeader.getWidth() / 2, cursorY, Z::UIText);
        cursorY += mImgTimeHeader.getHeight() + groupGap;

        int timeBgY = cursorY;
        mImgTimeBackground.draw(panelX, timeBgY, Z::UIPanel);
        mNumbers.draw(mTimeText,
            panelX + (mImgTimeBackground.getWidth() - mNumbers.width(mTimeText, timeFontSize)) / 2,
            timeBgY + (mImgTimeBackground.getHeight() - mNumbers.height(timeFontSize)) / 2,
            Z::UIText, timeFontSize, lcdColor);
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