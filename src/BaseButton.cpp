#include "BaseButton.h"

#include "go_textureatlas.h"
#include "Assets.h"
#include "ZOrder.h"

#include <algorithm>
#include <cctype>

BaseButton::BaseButton() { }


void BaseButton::set (GoSDL::Window * parentWindow, const BitmapFontAtlas * fonts,
                      std::string caption, std::string iconPath)
{
    mParentWindow = parentWindow;

    // The caption font is set up once here rather than per setText() call: the
    // old code reopened the .ttf every time the caption changed.
    mFontCaption.setAll(fonts, Assets::Font::Menu, 20);

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
    // Upper-case the caption (ASCII only, so multi-byte translated text isn't corrupted)
    std::transform(caption.begin(), caption.end(), caption.begin(),
        [](unsigned char c) { return std::toupper(c); });

    mCaption = std::move(caption);

    int captionWidth = mFontCaption.getTextWidth(mCaption);

    // Calculate the position of the text: leave a left-hand column as wide
    // as the icon, then center the caption within the remaining space
    if (mHasIcon)
    {
        int iconZoneWidth = mImgIcon.getWidth();

        mTextHorizontalPosition = iconZoneWidth + (mImgBackground.getWidth() - iconZoneWidth) / 2 - captionWidth / 2;
    }

    else
    {
        mTextHorizontalPosition = mImgBackground.getWidth() / 2 - captionWidth / 2;
    }
}


void BaseButton::draw(int x, int y, double z)
{
    mLastX = x;
    mLastY = y;

    if (mHasIcon)
    {
        int iconPad = (mImgBackground.getHeight() - mImgIcon.getHeight()) / 2;
        mImgIcon.draw(x + iconPad + 3, y + iconPad - 3, z + Z::Button::Icon);
    }

    int captionY = y + (mImgBackground.getHeight() - mFontCaption.getHeight()) / 2;
    mFontCaption.drawWithShadow(mCaption, x + mTextHorizontalPosition, captionY - 2,
        z + Z::Button::Caption, {255, 255, 255, 255}, 1, 2, {0, 0, 0, 128});

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
