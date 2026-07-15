#include "Util.h"

#include <cstdlib>
#include <SDL.h>

#if !defined(_WIN32) && !defined(__vita__)
    #include <dirent.h>
#endif


std::string getBasePath()
{
    std::string basePathStr;

    char *basePath = SDL_GetBasePath();
    if (basePath) {
        basePathStr = basePath;
        SDL_free(basePath);
    }

    #if !defined(_WIN32) && !defined(__vita__)
        // Check if game is installed system wide
        DIR* dir = opendir(std::string(basePathStr + "../share/seajeweled/").c_str());
        if (dir) {
            basePathStr += "../share/seajeweled/";
            closedir(dir);
        }
    #endif

    return basePathStr;
}

std::string getPrefPath()
{
    std::string prefPathStr;

    char *prefPath = SDL_GetPrefPath(NULL, "seajeweled");
    if (prefPath) {
        prefPathStr = prefPath;
        SDL_free(prefPath);
    }

    return prefPathStr;
}

float getRandomFloat(float a, float b)
{
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;

    return a + r;
}

int getRandomInt (int min, int max)
{
    return min + (rand() % (int)(max - min + 1));
}