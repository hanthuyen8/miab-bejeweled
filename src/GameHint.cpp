#include "GameHint.h"
#include "go_textureatlas.h"
#include "Assets.h"
#include "ZOrder.h"

void GameHint::setWindow (GoSDL::Window * w)
{
    // Use the shared atlas so the hint selector batches with the board sprites
    GoSDL::TextureAtlas atlas;
    atlas.load(w, Assets::AtlasImage, Assets::AtlasData);
    atlas.setImage(mImgSelector, Assets::Sprite::Selector);
}

void GameHint::showHint(Coord location)
{
    // Set the location
    mHintLocation = location;

    // Reset the animation
    mAnimationCurrentStep = 0;

    // Set the flag
    mShowingHint = true;
}

void GameHint::draw()
{
    // Don't draw if it's not necessary
    if (!mShowingHint)
        return;

    // Step the animation and check if it's finished
    if (mAnimationCurrentStep++ == mAnimationTotalSteps)
    {
        mShowingHint = false;
    }
    else
    {
        // Get the opacity percentage
        float p1 = 1 - (float)mAnimationCurrentStep / mAnimationTotalSteps;

        // Get the location
        float pX1 = 241 + mHintLocation.x * 65 - mImgSelector.getWidth() * (2 - p1) / 2 + 65 / 2;
        float pY1 = 41 + mHintLocation.y * 65 - mImgSelector.getHeight() * (2 - p1) / 2 + 65 / 2;

        // Draw the hint
        mImgSelector.draw(pX1, pY1, Z::Hint,
          2 - p1, 2 - p1,
          0, p1 * 255, {0, 255, 0, 255});
    }

}