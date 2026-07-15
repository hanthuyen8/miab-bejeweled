#ifndef _TEXTUREATLAS_H_
#define _TEXTUREATLAS_H_

#include <string>
#include <map>
#include <SDL.h>

#include "go_image.h"

namespace GoSDL {
    class Window;

    /**
     * Loads a TexturePacker "JSON (Hash)" atlas description and configures
     * GoSDL::Image instances to reference individual sub-sprites. All images
     * configured from the same atlas share a single SDL_Texture (via the
     * texture cache in GoSDL::Image), which lets the renderer batch their
     * draws into far fewer draw calls.
     */
    class TextureAtlas {
    public:

        /// Parse the frame table. imagePath/jsonPath are relative to the media
        /// base path, same convention as Image::setWindowAndPath().
        bool load(Window * window, const std::string & imagePath, const std::string & jsonPath);

        /// Configure `image` to draw the named sub-sprite (e.g. "gemRed.png").
        /// Returns false if the sprite name is not present in the atlas.
        bool setImage(Image & image, const std::string & spriteName) const;

        bool contains(const std::string & spriteName) const;

    private:
        Window * mWindow = nullptr;
        std::string mImagePath;
        std::map<std::string, SDL_Rect> mFrames;
    };
}

#endif /* _TEXTUREATLAS_H_ */
