#include "go_image.h"
#include "go_window.h"

#include "log.h"
#include "Util.h"

#include <map>

namespace {
    // Cache of loaded textures keyed by file path. Held via weak_ptr so a
    // texture is destroyed once the last Image referencing it goes away, but
    // while it is alive every Image loading the same path (e.g. an atlas)
    // shares the SAME SDL_Texture pointer — which is what lets the renderer
    // batch their draws together.
    std::map<std::string, std::weak_ptr<SDL_Texture>> sTextureCache;
}

GoSDL::Image::Image() : mParentWindow(NULL), mWidth(0), mHeight(0)
{ }

GoSDL::Image::Image(GoSDL::Window * parentWindow, string path) :
    mParentWindow(parentWindow), mPath(getBasePath() + path)
{
    loadTexture();
}

GoSDL::Image::Image (const Image & other)
{
    mParentWindow = other.mParentWindow;
    mPath = other.mPath;
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mTexture = other.mTexture;
    mSrcRect = other.mSrcRect;
    mHasSrcRect = other.mHasSrcRect;
}

GoSDL::Image::Image (Image && other)
{
    mParentWindow = other.mParentWindow;
    mPath = other.mPath;
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mTexture = other.mTexture;
    mSrcRect = other.mSrcRect;
    mHasSrcRect = other.mHasSrcRect;

    other.mTexture.reset();
}

GoSDL::Image::~Image()
{
    // Surface destruction happens in the custom deleter of the shared pointer
    mParentWindow = NULL;
}

GoSDL::Image & GoSDL::Image::operator= (GoSDL::Image&& other)
{
    if (this != &other)
    {
        mParentWindow = other.mParentWindow;
        mPath = other.mPath;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mTexture = other.mTexture;
        mSrcRect = other.mSrcRect;
        mHasSrcRect = other.mHasSrcRect;
    }

    return *this;
}

void GoSDL::Image::setWindow(GoSDL::Window * parentWindow)
{
    mParentWindow = parentWindow;
}

void GoSDL::Image::setPath(std::string path)
{
    mPath = getBasePath() + path;
    loadTexture();
}

bool GoSDL::Image::setWindowAndPath(GoSDL::Window * parentWindow, std::string path)
{
    mParentWindow = parentWindow;
    mPath = getBasePath() + path;
    return loadTexture();
}

bool GoSDL::Image::loadTexture()
{
    // A freshly (re)loaded image draws its whole texture until told otherwise.
    mHasSrcRect = false;

    // Reuse an already-loaded texture for the same path when possible, so that
    // several images sharing an atlas end up with the SAME SDL_Texture pointer.
    auto cached = sTextureCache.find(mPath);
    if (cached != sTextureCache.end())
    {
        if (auto existing = cached->second.lock())
        {
            mTexture = existing;
            SDL_QueryTexture(mTexture.get(), NULL, NULL, &mWidth, &mHeight);
            return true;
        }
    }

    // Load texture from file
    SDL_Texture * texture = IMG_LoadTexture(mParentWindow->getRenderer(), mPath.c_str());

    if (texture == nullptr)
    {
        return false;
    }

    // Fill the managed pointer and publish it to the cache
    mTexture.reset(texture, GoSDL::Image::SDL_Texture_Deleter());
    sTextureCache[mPath] = mTexture;

    // Get texture's width and height
    SDL_QueryTexture(mTexture.get(), NULL, NULL, &mWidth, &mHeight);

    return true;
}

void GoSDL::Image::setSrcRect(SDL_Rect srcRect)
{
    mSrcRect = srcRect;
    mHasSrcRect = true;

    // Report the region size so centering math (getWidth/getHeight) and the
    // destination rectangle in draw() behave as if this were a full sprite.
    mWidth = srcRect.w;
    mHeight = srcRect.h;
}

void GoSDL::Image::setTexture (SDL_Texture * texture)
{
    // Directly-assigned textures (e.g. rendered text) draw in full
    mHasSrcRect = false;

    // Assign the texture
    mTexture.reset(texture, GoSDL::Image::SDL_Texture_Deleter());

    // Get texture's width and height
    SDL_QueryTexture(mTexture.get(), NULL, NULL, &mWidth, &mHeight);
}

bool GoSDL::Image::getSrcRect(SDL_Rect & out) const
{
    if (!mHasSrcRect)
    {
        return false;
    }

    out = mSrcRect;
    return true;
}

int GoSDL::Image::getWidth()
{
    return mWidth;
}

int GoSDL::Image::getHeight()
{
    return mHeight;
}


bool GoSDL::Image::draw(int x, int y, int z, double factorX, double factorY, float angle, Uint8 alpha, SDL_Color color)
{
    if (mParentWindow == NULL)
    {
        lDEBUG << "Parent window NULL";
        return false;
    }

    if (mTexture == NULL)
    {
        lDEBUG << "Texture NULL";
        return false;
    }

    SDL_Rect destRect;
    destRect.w = mWidth * factorX;
    destRect.h = mHeight * factorY;
    destRect.x = x;
    destRect.y = y;

    mParentWindow->enqueueDraw(mTexture.get(), destRect, angle, z, alpha, color,
                               mHasSrcRect ? &mSrcRect : nullptr);

    return true;
}

