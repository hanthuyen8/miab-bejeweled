#include "go_textureatlas.h"
#include "go_window.h"

#include "Util.h"
#include "log.h"

#include <fstream>
#include <json/json.h>

bool GoSDL::TextureAtlas::load(GoSDL::Window * window, const std::string & imagePath, const std::string & jsonPath)
{
    mWindow = window;
    mImagePath = imagePath;
    mFrames.clear();

    std::ifstream stream(getBasePath() + jsonPath, std::ios::in);
    if (!stream.is_open())
    {
        lDEBUG << "Could not open atlas json: " << jsonPath;
        return false;
    }

    Json::CharReaderBuilder builder;
    Json::Value root;
    JSONCPP_STRING errs;

    if (!parseFromStream(builder, stream, &root, &errs))
    {
        lDEBUG << "Could not parse atlas json: " << errs;
        return false;
    }

    const Json::Value & frames = root["frames"];

    for (const auto & name : frames.getMemberNames())
    {
        const Json::Value & frame = frames[name]["frame"];

        SDL_Rect rect;
        rect.x = frame["x"].asInt();
        rect.y = frame["y"].asInt();
        rect.w = frame["w"].asInt();
        rect.h = frame["h"].asInt();

        mFrames[name] = rect;
    }

    return true;
}

bool GoSDL::TextureAtlas::contains(const std::string & spriteName) const
{
    return mFrames.find(spriteName) != mFrames.end();
}

bool GoSDL::TextureAtlas::setImage(GoSDL::Image & image, const std::string & spriteName) const
{
    auto it = mFrames.find(spriteName);

    if (it == mFrames.end())
    {
        lDEBUG << "Atlas sprite not found: " << spriteName;
        return false;
    }

    // Load (or reuse from cache) the shared atlas texture, then point this
    // image at its sub-region.
    if (!image.setWindowAndPath(mWindow, mImagePath))
    {
        return false;
    }

    image.setSrcRect(it->second);
    return true;
}
