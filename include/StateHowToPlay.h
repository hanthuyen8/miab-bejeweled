#ifndef _STATEHOWTOPLAY_H_
#define _STATEHOWTOPLAY_H_

#include <memory>

#include <string>
#include <vector>

#include "State.h"
#include "go_image.h"
#include "BitmapFont.h"

class TextBlock;
class Game;

class StateHowtoplay : public State {
public:

    StateHowtoplay(Game * p);

    void update();
    void draw();

    void buttonDown (SDL_Keycode button);
    void mouseButtonDown (Uint8 button);

    void controllerButtonDown(Uint8 button);

    ~StateHowtoplay();

private:
    /// Width the body copy wraps to
    static constexpr int kBodyWidth = 450;

    GoSDL::Image mImgBackground;

    BitmapFont mFontTitle;
    BitmapFont mFontSubtitle;
    BitmapFont mFontBody;

    std::string mTitleText;
    std::string mSubtitleText;

    /// Body copy, wrapped once at construction rather than every frame
    std::vector<std::string> mBodyLines;
};

#endif /* _STATEHOWTOPLAY_H_ */
