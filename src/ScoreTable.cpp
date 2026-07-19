#include "ScoreTable.h"
#include "Assets.h"
#include "inter.h"

#include "log.h"
#include "Game.h"

#include <fstream>

#ifdef _WIN32
    #include <io.h>
    #include <process.h>
#else
    #include <pwd.h>
    #include <unistd.h>
#endif

using namespace std;

ScoreTable::ScoreTable(Game * p, int score, string gameMode) : mGame(p)
{
    options.loadResources();

    int lastScore = 0;
    if (gameMode == "stateGameTimetrial") {
        lastScore = options.getHighscoreTimetrial();
        if (lastScore < score) {
            options.setHighscoreTimetrial(score);
        }
    } else if (gameMode == "stateGameEndless") {
        lastScore = options.getHighscoreEndless();
        if (lastScore < score) {
            options.setHighscoreEndless(score);
        }
    }

    scoreBoardWidth = 300;

    // Fonts for the three lines, and the text each one draws
    mFontHeader.setAll(mGame->getFonts(), Assets::Font::Menu, 60);
    mFontScore.setAll(mGame->getFonts(), Assets::Font::Menu, 72);
    mFontLastScore.setAll(mGame->getFonts(), Assets::Font::Normal, 35);

    mHeaderText = _("GAME OVER");
    mScoreText = std::to_string(score);
    mLastScoreText = _("Latest high score: ") + std::to_string(lastScore);
}

void ScoreTable::draw(int x, int y, int z)
{
    // Get the center
    int center = x + scoreBoardWidth / 2;

    const SDL_Color white = {255, 255, 255, 255}, shadow = {0, 0, 0, 128};

    // Draw the title and its shadow
    mFontHeader.drawWithShadow(mHeaderText,
        center - mFontHeader.getTextWidth(mHeaderText) / 2, y, z, white, 1, 3, shadow);

    // Draw the score and its shadow
    mFontScore.drawWithShadow(mScoreText,
        center - mFontScore.getTextWidth(mScoreText) / 2, y + 67 + 52, z, white, 1, 3, shadow);

    // Draw the previous high score and its shadow
    mFontLastScore.drawWithShadow(mLastScoreText,
        center - mFontLastScore.getTextWidth(mLastScoreText) / 2, y + 67 + 157, z, white, 1, 3, shadow);
}
