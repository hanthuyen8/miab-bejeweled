#ifndef GAME_INDICATORS_H
#define GAME_INDICATORS_H

#include "BitmapFont.h"
#include "go_image.h"
#include "go_music.h"

#include "BaseButton.h"

#include "OptionsManager.h"

#include <string>

class Game;
class StateGame;

class GameIndicators
{
public:
    GameIndicators();

    void setGame(Game *, StateGame *);

    void loadResources();

    /// Returns the current score
    int getScore();

    /// Sets the score to the given amount
    void setScore (int score);

    /// Increases the score by the given amount
    void increaseScore (int amount);

    /// Updates the remaining time, the argument is given in seconds
    void updateTime (double time);

    void disableTime();

    void enableTime();

    void draw();
    void click(int, int);

private:

    /// Regenerates the texture for the score, if necessary
    void regenerateScoreTexture();

    Game * mGame;
    StateGame * mStateGame;

    /// Current score
    int mScore = 0;

    /// Score in the previous frame
    int mScorePrevious = -1;

    /// Remaining time, in seconds
    double mRemainingTime = 0;

    /// Remaining time in the previous frame
    double mRemainingTimePrevious = 0;

    bool mTimeEnabled;

    /// Size the score and time readouts are drawn at
    static constexpr int kDigitFontSize = 30;

    /// Fonts for the readout digits and the labels above them
    BitmapFont mFontDigits;
    BitmapFont mFontHeader;

    /// Current score/time strings to draw (updated only when the value changes)
    std::string mScoreText = "0";
    std::string mTimeText;

    // Background of the timer
    GoSDL::Image mImgTimeBackground;

    /// Background for the current-score board
    GoSDL::Image mImgScoreBackground;

    /// Header labels, drawn from the atlas like everything else
    std::string mTimeHeaderText;
    std::string mScoreHeaderText;

    /// @{
    /// @name Buttons of the interface
    BaseButton mHintButton;
    BaseButton mResetButton;
    BaseButton mExitButton;
    /// @}

    /// Game music
    GoSDL::Music sfxSong;

    OptionsManager options;
};

#endif