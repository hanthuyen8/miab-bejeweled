#include "GameBoard.h"

#include "Game.h"
#include "StateGame.h"
#include "go_textureatlas.h"
#include "Assets.h"
#include "ZOrder.h"
#include "MiabSDK.h"
#include <cmath>
#include <functional>

using namespace std::placeholders;

namespace {
    // How often the highlight crosses the board
    constexpr Uint32 kGlintCycleMs = 14000;

    // The one speed knob: how long the light takes to travel one cell width.
    // Everything else below is derived from it, because the highlight has to be
    // a single straight line crossing the board — if the speed inside a gem and
    // the speed from gem to gem are set independently they drift apart, and it
    // stops reading as one light and starts reading as gems blinking on their
    // own. Smaller is faster.
    constexpr Uint32 kGlintCellTravelMs = 160;

    // One sweep covers the cell plus the run-up and run-out either side of it,
    // at that same speed.
    constexpr Uint32 kGlintSweepMs =
        kGlintCellTravelMs * GemShine::kSweepTravel / GemShine::kCellSize;
}


void GameBoard::setGame(Game * game, StateGame * stateGame)
{
    mGame = game;
    mStateGame = stateGame;

    mBoard.generate();

    mState = eBoardAppearing;
    mAnimationCurrentStep = 0;
}

void GameBoard::resetGame()
{
    // Game can only be reset on the steady state, or when the Game.has ended
    if (mState != eSteady && mState != eShowingScoreTable)
        return;

    // Reset the variables
    mMultiplier = 0;
    mAnimationCurrentStep = 0;

    // If there's a board on screen, make it disappear first
    if (mState != eShowingScoreTable)
    {
        // Drop all gems
        mBoard.dropAllGems();

        // Switch state
        mState = eBoardDisappearing;
    }

    // Otherwise, directly generate and show the new board
    else {
        // Generate a brand new board
        mBoard.generate();

        // Switch state
        mState = eBoardAppearing;
    }

}

void GameBoard::endGame(int score, int elapsedMs)
{
    if (mState == eTimeFinished || mState == eShowingScoreTable)
        return;

    mBoard.dropAllGems();
    mState = eTimeFinished;

    // Generate the score table
    scoreTable = std::make_shared<ScoreTable>(mGame, score, mGame->getCurrentState());

    // Report the highscore to the MIAB host page, if embedded
    std::string currentState = mGame->getCurrentState();
    if (currentState == "stateGameEndless")
    {
        MiabSDK::SubmitHighscore(score, "endless");
    }
    else if (currentState == "stateGameTimetrial")
    {
        MiabSDK::SubmitHighscore(score, "timetrial", elapsedMs);
    }
}

#ifdef SEAJEWELED_CHEATS

void GameBoard::showScoreTableForTesting(int score)
{
    if (mState == eShowingScoreTable)
        return;

    scoreTable = std::make_shared<ScoreTable>(mGame, score, mGame->getCurrentState());
    mState = eShowingScoreTable;
}

#endif

void GameBoard::loadResources()
{
    // All board sprites live in a single texture atlas, so every gem, the
    // selector and the particles share one SDL_Texture and the renderer can
    // batch their draws (see go_textureatlas.h / the texture cache in Image).
    GoSDL::TextureAtlas atlas;
    atlas.load(mGame, Assets::AtlasImage, Assets::AtlasData);

    atlas.setImage(mImgWhite,  Assets::Sprite::GemWhite);
    atlas.setImage(mImgRed,    Assets::Sprite::GemRed);
    atlas.setImage(mImgPurple, Assets::Sprite::GemPurple);
    atlas.setImage(mImgOrange, Assets::Sprite::GemOrange);
    atlas.setImage(mImgGreen,  Assets::Sprite::GemGreen);
    atlas.setImage(mImgYellow, Assets::Sprite::GemYellow);
    atlas.setImage(mImgBlue,   Assets::Sprite::GemBlue);

    // Re-point the gems at the pre-composited sheet holding the sweeping
    // highlight. It has its own texture, so gems no longer batch together with
    // the rest of the atlas — but they still share one texture among
    // themselves, so the number of draw calls is unchanged.
    if (mGemShine.bake(mGame, atlas))
    {
        mGemShine.attach(mImgWhite,  GemShine::White);
        mGemShine.attach(mImgRed,    GemShine::Red);
        mGemShine.attach(mImgPurple, GemShine::Purple);
        mGemShine.attach(mImgOrange, GemShine::Orange);
        mGemShine.attach(mImgGreen,  GemShine::Green);
        mGemShine.attach(mImgYellow, GemShine::Yellow);
        mGemShine.attach(mImgBlue,   GemShine::Blue);
    }

    // The square selector
    atlas.setImage(mImgSelector, Assets::Sprite::Selector);

    // The particles
    atlas.setImage(mImgParticle1, Assets::Sprite::Particle1);
    atlas.setImage(mImgParticle2, Assets::Sprite::Particle2);

    // Font the floating scores draw with, from the same atlas
    mFontFloatingScore.setAll(mGame->getFonts(), Assets::Font::Lcd, 60);

    // Initialise the hint
    mHint.setWindow(mGame);

    // Initialise the sounds
    mGame->getGameSounds()->loadResources();
}

void GameBoard::update()
{
    // Keep the selector spinning and gently pulsing regardless of state,
    // advancing by elapsed real time so the motion stays smooth even when
    // update() isn't called at a perfectly steady rate (see GameBoard.h).
    Uint32 nowTicks = SDL_GetTicks();
    Uint32 deltaMs = mSelectorLastTicks == 0 ? 16 : nowTicks - mSelectorLastTicks;
    if (deltaMs > 100) deltaMs = 100; // clamp huge gaps (e.g. tab was backgrounded)
    mSelectorLastTicks = nowTicks;
    double deltaSteps = deltaMs / 16.6667; // relative to a 60fps reference step

    mSelectorAngle = std::fmod(mSelectorAngle + 1.0 * deltaSteps, 360.0);
    mSelectorPulsePhase += 0.025 * deltaSteps;

    // Default state, do nothing
    if (mState == eSteady)
    {
        mMultiplier = 0;
        mAnimationCurrentStep = 0;
    }

    // Board appearing, gems are falling
    else if (mState == eBoardAppearing)
    {
        // Update the animation frame
        mAnimationCurrentStep ++;

        // If the Animation.has finished, switch to steady state
        if (mAnimationCurrentStep == mAnimationLongTotalSteps)
        {
            mState = eSteady;
        }
    }

    // Two winning gems are switching places
    else if (mState == eGemSwitching)
    {
        // Update the animation frame
        mAnimationCurrentStep ++;

        // If the Animation.has finished, matching gems should disappear
        if (mAnimationCurrentStep == mAnimationShortTotalSteps)
        {
            // Winning games should disappear
            mState = eGemDisappearing;

            // Reset the animation
            mAnimationCurrentStep = 0;

            // Swap the gems in the board
            mBoard.swap(mSelectedSquareFirst.x, mSelectedSquareFirst.y,
                        mSelectedSquareSecond.x, mSelectedSquareSecond.y);

            // Increase the mMultiplier
            ++mMultiplier;

            // Play matching sound
            playMatchSound();

            // Create floating scores for the matching group
            createFloatingScores();
        }
    }

    // Matched gems are disappearing
    else if (mState == eGemDisappearing)
    {
        // Update the animation frame
        mAnimationCurrentStep ++;

        // If the Animation.has finished
        if (mAnimationCurrentStep == mAnimationShortTotalSteps)
        {
            // Empty spaces should be filled with new gems
            mState = eBoardFilling;

            // Delete the squares that were matched in the board
            for(size_t i = 0; i < mGroupedSquares.size(); ++i)
            {
                for(size_t j = 0; j < mGroupedSquares[i].size(); ++j)
                {
                    mBoard.del(mGroupedSquares[i][j].x, mGroupedSquares[i][j].y);
                }
            }

            // Calculate fall movements
            mBoard.calcFallMovements();

            // Reset the animation
            mAnimationCurrentStep = 0;
        }
    }

    // New gems are falling to their proper places
    else if (mState == eBoardFilling)
    {
        // Update the animation frame
        mAnimationCurrentStep ++;

        // If the Animation.has finished
        if (mAnimationCurrentStep == mAnimationShortTotalSteps)
        {
            // Play the fall sound
            mGame->getGameSounds()->playSoundFall();

            // Switch to the normal state
            mState = eSteady;

            // Allow getting points again if a hint was used
            mHintUsed = false;

            // Reset the animation
            mAnimationCurrentStep = 0;

            // Reset animations in the board
            mBoard.endAnimations();

            // Check if there are matching groups
            mGroupedSquares = mBoard.check();

            // If there are...
            if (! mGroupedSquares.empty())
            {
                // Increase the score mMultiplier
                ++mMultiplier;

                // Create the floating scores
                createFloatingScores();

                // Play matching sound
                playMatchSound();

                // Go back to the gems-fading mState
                mState = eGemDisappearing;
            }

            // If there are neither current solutions nor possible future solutions
            else if (mBoard.solutions().empty())
            {
                if (mGame->getCurrentState() == "stateGameEndless") {
                    endGame(mStateGame->getScore());
                } else {
                    // Make the board disappear
                    mState = eBoardDisappearing;

                    // Make all the gems want to go outside the board
                    mBoard.dropAllGems();
                }
            }
        }
    }

    // The entire board is disappearing to show a new one
    else if (mState == eBoardDisappearing)
    {
        // Update the animation frame
        mAnimationCurrentStep ++;

        // If the Animation.has finished
        if (mAnimationCurrentStep == mAnimationLongTotalSteps)
        {
            // Reset animation counter
            mAnimationCurrentStep = 0;

            // Generate a brand new board
            mBoard.generate();

            // Switch state
            mState = eBoardAppearing;
        }
    }

    // The board is disappearing because the time has run out
    else if (mState == eTimeFinished)
    {
        // Update the animation frame
        mAnimationCurrentStep ++;

        // If the Animation.has finished
        if (mAnimationCurrentStep == mAnimationLongTotalSteps)
        {
            // Reset animation counter
            mAnimationCurrentStep = 0;

            // Switch state
            mState = eShowingScoreTable;
        }
    }

    // Remove the hidden floating score
    mFloatingScores.erase(
        remove_if (mFloatingScores.begin(),
                   mFloatingScores.end(),
                   std::bind<bool>(&FloatingScore::ended, _1)),
        mFloatingScores.end());

    // Remove the hiden particle systems
    mParticleSet.erase(
        remove_if (mParticleSet.begin(),
                   mParticleSet.end(),
                   std::bind<bool>(&ParticleSystem::ended, _1)),
        mParticleSet.end());
}

void GameBoard::draw()
{
    if (mGame->getMouseActive()) {
        // Get mouse position
        int mX = (int) mGame -> getMouseX();
        int mY = (int) mGame -> getMouseY();

        // Move the selector to the mouse if it is over a gem
        if (overGem(mX, mY))
        {
                Coord mouseCoords = getCoord(mX, mY);
                mSelectorX = mouseCoords.x;
                mSelectorY = mouseCoords.y;
        }
    }

    // Gentle continuous fade in/out, kept within a subtle, visible range
    Uint8 selectorAlpha = Uint8(200 + 55 * std::sin(mSelectorPulsePhase));

    // The selector art is bigger than a gem cell (65px) so its ring shape
    // isn't clipped by the cell edges; center it on the cell by offsetting
    // by half the size difference.
    int selectorInset = (mImgSelector.getWidth() - 65) / 2;

    // Draw the selector over that gem
    mImgSelector.draw(
        241 + mSelectorX * 65 - selectorInset,
        41 + mSelectorY * 65 - selectorInset,
        Z::Selector, 1, 1, mSelectorAngle, selectorAlpha);

    // Draw the selector if a gem has been selected
    if (mState == eGemSelected)
    {
        mImgSelector.draw(241 + mSelectedSquareFirst.x * 65 - selectorInset,
              41 + mSelectedSquareFirst.y * 65 - selectorInset,
              Z::Selector, 1, 1, mSelectorAngle, selectorAlpha, {0, 255, 255, 255});
    }

    // Draw the hint
    mHint.draw();

    // Draw each floating score
    std::for_each(mFloatingScores.begin(),
      mFloatingScores.end(),
      std::bind(&FloatingScore::draw, _1));

    // Draw each particle system
    std::for_each(mParticleSet.begin(),
      mParticleSet.end(),
      std::bind(&ParticleSystem::draw, _1));

    // If game has finished, draw the score table
    if (mState == eShowingScoreTable)
    {
        scoreTable -> draw(241 + (65 * 8) / 2 - 150  , 105, Z::ScoreTable);
    }


    // On to the gem drawing procedure. Let's have a pointer to the image of each gem
    GoSDL::Image * img = NULL;

    // The highlight only sweeps while the board is at rest: during a swap or a
    // cascade the gems are already carrying the eye, and a second moving
    // highlight on top of that just reads as noise.
    Uint32 nowTicks = SDL_GetTicks();
    bool glintActive = (mState == eSteady || mState == eGemSelected);

    // Top left position of the board
    int posX = 241;
    int posY = 41;

    for(int i = 0; i < 8; ++i)
    {
        for(int j = 0; j < 8; ++j)
        {
            // Reset the pointer
            img = NULL;
            GemShine::Gem gemColour = GemShine::White;

            // Check the type of each square and
            // save the proper image in the img pointer
            switch(mBoard.squares[i][j])
            {
                case sqWhite:
                img = &mImgWhite;
                gemColour = GemShine::White;
                break;

                case sqRed:
                img = &mImgRed;
                gemColour = GemShine::Red;
                break;

                case sqPurple:
                img = &mImgPurple;
                gemColour = GemShine::Purple;
                break;

                case sqOrange:
                img = &mImgOrange;
                gemColour = GemShine::Orange;
                break;

                case sqGreen:
                img = &mImgGreen;
                gemColour = GemShine::Green;
                break;

                case sqYellow:
                img = &mImgYellow;
                gemColour = GemShine::Yellow;
                break;

                case sqBlue:
                img = &mImgBlue;
                gemColour = GemShine::Blue;
                break;

                case sqEmpty:
                img = NULL;
                break;
            }

            if (img == NULL)
            {
                continue;
            }

            float imgX = posX + i * 65;
            float imgY = posY + j * 65;
            float imgAlpha = 255;

            // When the board is first appearing, all the gems are falling
            if (mState == eBoardAppearing)
            {
                imgY = Animacion::easeOutQuad(
                        float(mAnimationCurrentStep),
                        float(posY + mBoard.squares[i][j].origY * 65),
                        float(mBoard.squares[i][j].destY * 65),
                        float(mAnimationLongTotalSteps));
            }

            // When two correct gems have been selected, they switch positions
            else if (mState == eGemSwitching)
            {
                // If the current gem is the first selected square
                if (mSelectedSquareFirst.equals(i,j))
                {
                    imgX = Animacion::easeOutQuad(
                        float(mAnimationCurrentStep),
                        float(posX + i * 65),
                        float((mSelectedSquareSecond.x - mSelectedSquareFirst.x) * 65),
                        float(mAnimationShortTotalSteps));

                    imgY = Animacion::easeOutQuad(
                        float(mAnimationCurrentStep),
                        float(posY + j * 65),
                        float((mSelectedSquareSecond.y - mSelectedSquareFirst.y) * 65),
                        float(mAnimationShortTotalSteps));
                }

                // If the current gem is the second selected square
                else if (mSelectedSquareSecond.equals(i,j))
                {
                    imgX = Animacion::easeOutQuad(
                        float(mAnimationCurrentStep),
                        float(posX + i * 65),
                        float((mSelectedSquareFirst.x - mSelectedSquareSecond.x) * 65),
                        float(mAnimationShortTotalSteps));

                    imgY = Animacion::easeOutQuad(
                        float(mAnimationCurrentStep),
                        float(posY + j * 65),
                        float((mSelectedSquareFirst.y - mSelectedSquareSecond.y) * 65),
                        float(mAnimationShortTotalSteps));
                }
            }

            // When the two selected gems have switched, the matched gems disappear
            else if (mState == eGemDisappearing)
            {
                if (mGroupedSquares.matched(Coord(i, j)))
                {
                    imgAlpha = 255 * (1 -(float)mAnimationCurrentStep/mAnimationShortTotalSteps);
                }
            }

            // When matched gems have disappeared, spaces in the board must be filled
            else if (mState == eBoardFilling)
            {
                if (mBoard.squares[i][j].mustFall)
                {
                    imgY = Animacion::easeOutQuad(
                        float(mAnimationCurrentStep),
                        float(posY + mBoard.squares[i][j].origY * 65),
                        float(mBoard.squares[i][j].destY * 65),
                        float(mAnimationShortTotalSteps));
                }
            }

            // When there are no more matching movements, the board disappears
            else if (mState == eBoardDisappearing || mState == eTimeFinished)
            {
                imgY = Animacion::easeInQuad(
                        float(mAnimationCurrentStep),
                        float(posY + mBoard.squares[i][j].origY * 65),
                        float(mBoard.squares[i][j].destY * 65),
                        float(mAnimationLongTotalSteps));
            }

            else if (mState == eShowingScoreTable)
            {
                continue;
            }

            // Send a highlight sweeping across the gem every so often. Each
            // cell gets its own offset into the cycle, otherwise the whole
            // board glints in unison and reads as a screen-wide flash. Only
            // the source rectangle changes, so this costs nothing extra.
            int glintFrame = 0;

            if (glintActive)
            {
                // Where this cell sits along the light's path, in cell widths,
                // counted so that the first cell reached gets the largest
                // offset and therefore enters the sweep window first.
                //
                // The row term is what keeps the highlight straight: the streak
                // leans, so its lower end trails behind its upper end, and a
                // cell one row down has to be lit that much later. Without it
                // whole columns light at once and the line comes out as a
                // staircase — invisible while the streak is near vertical, and
                // increasingly wrong as it tilts.
                double rowLean = -GemShine::streakLean();
                double phaseCells = (7 - i) + rowLean * (7 - j);

                Uint32 cellPhase = Uint32(phaseCells * kGlintCellTravelMs);
                Uint32 cellTime = (nowTicks + cellPhase) % kGlintCycleMs;

                if (cellTime < kGlintSweepMs)
                {
                    glintFrame = int(cellTime * GemShine::kFrameCount / kGlintSweepMs);
                }
            }

            // Always set it, so a gem left mid-sweep when the board starts
            // moving goes back to its plain frame instead of freezing lit up
            mGemShine.setFrame(*img, gemColour, glintFrame);

            img->draw(imgX, imgY, Z::Gem, 1, 1, 0, imgAlpha);
        }


    }
}

void GameBoard::moveSelector(int x, int y) {
    mSelectorX+=x;
    mSelectorY+=y;

    if (mSelectorX < 0) {
        mSelectorX = 7;
    } else if (mSelectorY < 0) {
        mSelectorY = 7;
    } else if (mSelectorX > 7) {
        mSelectorX = 0;
    } else if (mSelectorY > 7) {
        mSelectorY = 0;
    }
}

void GameBoard::buttonDown(SDL_Keycode button)
{
    switch(button) {
    case SDLK_LEFT:
        mGame->getGameSounds()->playSoundSelect();
        moveSelector(-1, 0);
        break;
    case SDLK_RIGHT:
        mGame->getGameSounds()->playSoundSelect();
        moveSelector(1, 0);
        break;
    case SDLK_UP:
        mGame->getGameSounds()->playSoundSelect();
        moveSelector(0, -1);
        break;
    case SDLK_DOWN:
        mGame->getGameSounds()->playSoundSelect();
        moveSelector(0, 1);
        break;
    case SDLK_SPACE:
        selectGem();
        break;
    }
}

void GameBoard::controllerButtonDown(Uint8 button)
{
    switch (button)
    {
        case SDL_CONTROLLER_BUTTON_Y:
            showHint();
            break;

        case SDL_CONTROLLER_BUTTON_A:
            selectGem();
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            mGame->getGameSounds()->playSoundSelect();
            moveSelector(0, 1);
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            mGame->getGameSounds()->playSoundSelect();
            moveSelector(-1, 0);
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            mGame->getGameSounds()->playSoundSelect();
            moveSelector(0, -1);
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            mGame->getGameSounds()->playSoundSelect();
            moveSelector(1, 0);
            break;
    }
}


void GameBoard::mouseButtonDown(int mouseX, int mouseY)
{
    // A gem was clicked
    if (overGem(mouseX, mouseY))
    {
        Coord mouseCoords = getCoord(mouseX, mouseY);
        mSelectorX = mouseCoords.x;
        mSelectorY = mouseCoords.y;
        selectGem();
    }
}

void GameBoard::mouseButtonUp(int mX, int mY)
{
    if (mState == eGemSelected)
    {
        // Get the coordinates where the mouse was released
        Coord res = getCoord(mX, mY);

        // If the square is different from the previously selected one
        if (res != mSelectedSquareFirst && checkSelectedSquare())
        {
            // Switch the state and reset the animation
            mState = eGemSwitching;
            mAnimationCurrentStep = 0;
        }
    }
}

void GameBoard::selectGem() {
    mGame->getGameSounds()->playSoundSelect();

        if (mState == eSteady)
        {
            mState = eGemSelected;

            mSelectedSquareFirst.x = mSelectorX;
            mSelectedSquareFirst.y = mSelectorY;
        }

        // If there was previous a gem selected
        else if (mState == eGemSelected)
        {
            // If the newly clicked gem is a winning one
            if (checkSelectedSquare())
            {
                // Switch the state and reset the animation
                mState = eGemSwitching;
                mAnimationCurrentStep = 0;
            }
            else
            {
                mState = eSteady;
                mSelectedSquareFirst.x = -1;
                mSelectedSquareFirst.y = -1;
            }
        }
}

void GameBoard::showHint ()
{
    if (mState == eSteady) {
        // Make sure no points can be earned next turn
        mHintUsed = true;

        // Get possible hint locations
        vector<Coord> hintLocations = mBoard.solutions();

        // Start hint animation
        mHint.showHint(hintLocations[0]);
    }
}

void GameBoard::createFloatingScores()
{
    //Do not grant points if a hint was used
    int pointsPerGem = 5;
    if (mHintUsed) {
        pointsPerGem = 0;
    }

    // For each match in the group of matched squares
    for (Match & m : mGroupedSquares)
    {
        int score = m.size() * pointsPerGem * mMultiplier;

        // Create a new floating score image
        mFloatingScores.emplace_back(FloatingScore(&mFontFloatingScore,
           score,
           m.midSquare().x,
           m.midSquare().y, Z::FloatingScore));

        // Create a new particle system for it to appear over the square
        for(size_t i = 0, s = m.size(); i < s; ++i)
        {
            mParticleSet.emplace_back(ParticleSystem(&mImgParticle1, &mImgParticle2,
                50, 50,
                241 + m[i].x * 65 + 32,
                41 + m[i].y * 65 + 32, 60, 0.5));

        }

        mStateGame->increaseScore(score);
    }
}

void GameBoard::playMatchSound()
{
    if (mMultiplier == 1)
    {
        mGame->getGameSounds()->playSoundMatch1();
    }
    else if (mMultiplier == 2)
    {
        mGame->getGameSounds()->playSoundMatch2();
    }
    else
    {
        mGame->getGameSounds()->playSoundMatch3();
    }
}

/// Tests if the mouse is over a gem
bool GameBoard::overGem (int mX, int mY)
{
    return (mX > 241 && mX < 241 + 65 * 8 &&
        mY > 41 && mY < 41 + 65 * 8);
}

/// Returns the coords of the gem the mouse is over
Coord GameBoard::getCoord (int mX, int mY)
{
    return Coord((mX - 241) / 65 ,
       (mY - 41) / 65 );
}

bool GameBoard::checkSelectedSquare() {
    // Get the selected square
    mSelectedSquareSecond = getCoord(241 + mSelectorX * 65, 41 + mSelectorY * 65);

    // If it's a contiguous square
    if (abs(mSelectedSquareFirst.x - mSelectedSquareSecond.x) +
        abs(mSelectedSquareFirst.y - mSelectedSquareSecond.y) == 1)
    {
        // Create a temporal board with the movement already performed
        Board temporal = mBoard;
        temporal.swap(mSelectedSquareFirst.x, mSelectedSquareFirst.y,
                      mSelectedSquareSecond.x, mSelectedSquareSecond.y);

        // Check if there are grouped gems in that new board
        mGroupedSquares = temporal.check();

        // If there are winning movements
        if (! mGroupedSquares.empty())
        {
            return true;
        }
    }

    return false;
}
