#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <SDL.h>
#include <SDL_image.h>

#include <string>
#include <memory>
using namespace std;

#include "log.h"

namespace GoSDL {
    class Window;

    class Image {

    public:

        Image ();
        Image (Window * parentWindow, string path);
        ~Image ();

        Image (const Image & other);
        Image (Image && other);
        Image & operator= (Image&& other);

        void setWindow (Window * parentWindow);
        void setPath (string path);
        bool setWindowAndPath(Window * parentwindow, string path);
        void setTexture (SDL_Texture * texture);

        /// Restrict this image to a sub-region of its texture (an atlas frame).
        /// getWidth()/getHeight() and draw() then behave as if this region were
        /// a standalone sprite, so callers need no changes.
        void setSrcRect (SDL_Rect srcRect);

        bool draw (int x, int y, int z,
            double factorX = 1, double factorY = 1, float angle = 0,
            Uint8 alpha = 255, SDL_Color color = {255, 255, 255, 255});

        int getWidth();
        int getHeight();

    private:
        bool loadTexture();


        /*
            This class holds a pointer to a SDL_Texture. It used
            to destroy that texture in the destructor, but that
            eventually avoided this class from being copyable,
            because whenever I returned an Image from a function by copy,
            the instance in the function internally destroyed the
            SDL_Texture. Furthermore, SDL2 does not currently
            offer any easy way of duplicating a SDL_Texture, so
            using a copy constructor was not possible.

            The best option I've found is to use a shared_ptr to the
            texture instead of a traditional pointer, and add the proper
            texture deletion code in a custom deleter that integrates
            in the shared pointer. Clever, isn't it?
        */

        shared_ptr<SDL_Texture> mTexture;

        struct SDL_Texture_Deleter
        {
            void operator()(SDL_Texture * texture) const
            {
                if (texture)
                {
                    SDL_DestroyTexture(texture);
                }
            }
        };

        Window * mParentWindow = NULL;
        int mWidth, mHeight;

        // Sub-region of mTexture to draw (used for atlas frames). When
        // mHasSrcRect is false the whole texture is drawn.
        SDL_Rect mSrcRect = {0, 0, 0, 0};
        bool mHasSrcRect = false;

        string mPath;
    };
}

#endif /* _IMAGE_H_ */
