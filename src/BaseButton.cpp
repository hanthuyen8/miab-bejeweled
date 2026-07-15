#include "BaseButton.h"

#include "go_font.h"
#include "go_textureatlas.h"
#include "Assets.h"
#include "ZOrder.h"

BaseButton::BaseButton() { }


void BaseButton::set (GoSDL::Window * parentWindow, std::string caption, std::string iconPath)
{
    mParentWindow = parentWindow;

    // Background and icon come from the shared atlas so they batch with the
    // rest of the UI/board sprites instead of using their own textures.
    GoSDL::TextureAtlas atlas;
    atlas.load(mParentWindow, Assets::AtlasImage, Assets::AtlasData);

    // Load the background image
    atlas.setImage(mImgBackground, Assets::Sprite::ButtonBackground);

    // Set the flag
    mHasIcon = iconPath != "";

    // Load the icon image
    if (mHasIcon)
    {
        atlas.setImage(mImgIcon, iconPath);
    }

    setText(caption);
}

void BaseButton::setText(std::string caption)
{
    // Load the font for the button caption
    GoSDL::Font textFont;
    textFont.setAll(mParentWindow, Assets::FontNormal, 27);

    // Generate the button caption texture
    mImgCaption = textFont.renderTextWithShadow(caption, {255, 255, 255, 255}, 1, 2, {0, 0, 0, 128});

    // Calculate the position of the text
    if (mHasIcon)
    {
        mTextHorizontalPosition = 40 + (mImgBackground.getWidth() - 40) / 2 - mImgCaption.getWidth() / 2;
    }

    else
    {
        mTextHorizontalPosition = mImgBackground.getWidth() / 2 - mImgCaption.getWidth() / 2;
    }
}


void BaseButton::draw(int x, int y, double z)
{
    mLastX = x;
    mLastY = y;

    if (mHasIcon)
    {
        mImgIcon.draw(x + 7, y, z + Z::Button::Icon);
    }

    mImgCaption.draw(x + mTextHorizontalPosition, y + 5, z + Z::Button::Caption);

    mImgBackground.draw(x, y, z);

}

bool BaseButton::clicked(unsigned int mX, unsigned int mY)
{
    if(mX > mLastX && mX < mLastX + mImgBackground.getWidth() &&
       mY > mLastY && mY < mLastY + mImgBackground.getHeight())
       {
        return true;
    }else
    {
        return false;
    }
}
