#ifndef _JEWELGROUPANIM_H_
#define _JEWELGROUPANIM_H_

#include "Animation.h"

#include <memory>

#include "go_window.h"
#include "go_image.h"
#include "go_textureatlas.h"
#include "Assets.h"
#include "ZOrder.h"

class JewelGroupAnim
{
public:
    void loadResources (GoSDL::Window * w)
    {
        // Gems come from the shared atlas (same texture as the in-game board)
        GoSDL::TextureAtlas atlas;
        atlas.load(w, Assets::AtlasImage, Assets::AtlasData);
        atlas.setImage(imgGems[0], Assets::Sprite::GemWhite);
        atlas.setImage(imgGems[1], Assets::Sprite::GemRed);
        atlas.setImage(imgGems[2], Assets::Sprite::GemPurple);
        atlas.setImage(imgGems[3], Assets::Sprite::GemOrange);
        atlas.setImage(imgGems[4], Assets::Sprite::GemGreen);
        atlas.setImage(imgGems[5], Assets::Sprite::GemYellow);
        atlas.setImage(imgGems[6], Assets::Sprite::GemBlue);

        for (int i = 0; i < 7; ++i)
        {
            posX[i] = 800 / 2 - (65 * 7) / 2 + i * 65;
        }

        animationCurrentStep = 0;
        animationTotalSteps = 30;
        posFinalY = 265;
    }

    void draw(){

        // Step the animation
        if(animationCurrentStep < 7 * 5 + animationTotalSteps) {
            ++animationCurrentStep;
        }

        // Draw the jewels
        for(int i = 0; i < 7; ++i)
        {
            int composedStep = animationCurrentStep - i * 5;
            if(composedStep < 0) continue;

            if(composedStep < animationTotalSteps){
                imgGems[i].draw(posX[i],
                                   Animacion::easeOutCubic(
                                       (float) composedStep,
                                       600.f,
                                       (float) posFinalY - 600.f,
                                       (float) animationTotalSteps),
                                   Z::Menu::Jewel);
            }else{
                imgGems[i].draw(posX[i], posFinalY, Z::Menu::Jewel);
            }
        }
    }

private:
    GoSDL::Image imgGems[7];

    int posX[7], posFinalY;

    int animationCurrentStep;
    int animationTotalSteps;

};


#endif /* _JEWELGROUPANIM_H_ */
