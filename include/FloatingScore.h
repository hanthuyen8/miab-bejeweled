/**
 * @file FloatingScore.h
 *
 * @author José Tomás Tocino García
 * @date 2010
 *
 * Copyright (C) 2010 José Tomás Tocino García <theom3ga@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */


#ifndef _FLOATINGSCORE_H_
#define _FLOATINGSCORE_H_

#include <string>
using namespace std;

#include "log.h"
#include "go_image.h"
#include "BitmapNumber.h"
#include "Assets.h"

namespace GoSDL {
  class Window;
}

/**
 * The score that floats up off a match.
 *
 * Drawn with glyphs from the shared atlas rather than rasterized with SDL_ttf.
 * Rendering the text gave every score its own SDL_Texture — and a second one
 * for its shadow — so each one on screen forced its own draw call, right
 * during the particle burst. Atlas glyphs batch with everything else instead,
 * and nothing is rasterized or allocated while the board is animating.
 */
class FloatingScore{
public:
    /// `numbers` must outlive this object; GameBoard owns it.
    FloatingScore(BitmapNumber * numbers, int score, float x, float y, float z) :
        mNumbers(numbers), mScoreText(std::to_string(score)),
        x_(x), y_(y), z_(z), mCurrentStep(0), mTotalSteps(50) {
    }

    bool ended(){
        return mCurrentStep == mTotalSteps;
    }

    void draw(){
        if(mCurrentStep >= mTotalSteps) return;

        mCurrentStep += 1;

        float p = 1.f - (float)mCurrentStep/mTotalSteps;

        int posX = (int)(241 + x_ * 65);
        int posY = (int)(41 + y_ * 65 - (1 - p) * 20);

        Uint8 alpha = (Uint8)(p * 255);

        // Same glyphs, tinted black and offset both ways, for the outline
        mNumbers->draw(mScoreText, posX + 2, posY + 2, z_ - 0.1, kFontSize, {0, 0, 0, 255}, alpha);
        mNumbers->draw(mScoreText, posX - 2, posY - 2, z_ - 0.1, kFontSize, {0, 0, 0, 255}, alpha);

        mNumbers->draw(mScoreText, posX, posY, z_, kFontSize, {255, 255, 255, 255}, alpha);
    }
private:
    /// Matches the size the score used to be rasterized at
    static constexpr int kFontSize = 60;

    BitmapNumber * mNumbers;
    std::string mScoreText;

    float x_;
    float y_;
    float z_;

    int mCurrentStep;
    int mTotalSteps;


};

#endif /* _FLOATINGSCORE_H_ */
