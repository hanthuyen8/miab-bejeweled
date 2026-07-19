#include "GameSounds.h"
#include "Assets.h"

void GameSounds::loadResources()
{
    options.loadResources();

    if (options.getSoundEnabled() && !soundsLoaded) {
        mSfxMatch1.setSample(Assets::SfxMatch1);
        mSfxMatch2.setSample(Assets::SfxMatch2);
        mSfxMatch3.setSample(Assets::SfxMatch3);
        mSfxSelect.setSample(Assets::SfxSelect);
        mSfxFall.setSample(Assets::SfxFall);
        mSfxButtonHover.setSample(Assets::SfxButtonHover);
        mSfxButtonClick.setSample(Assets::SfxButtonClick);
        mSfxError.setSample(Assets::SfxError);

        soundsLoaded = true;
    } else if (!options.getSoundEnabled() && soundsLoaded) {
        mSfxMatch1.unload();
        mSfxMatch2.unload();
        mSfxMatch3.unload();
        mSfxSelect.unload();
        mSfxFall.unload();
        mSfxButtonHover.unload();
        mSfxButtonClick.unload();
        mSfxError.unload();

        soundsLoaded = false;
    }

}

void GameSounds::playSoundSelect()
{
        mSfxSelect.play(0.3);
}

void GameSounds::playSoundFall()
{
    mSfxFall.play(0.6);
}

void GameSounds::playSoundMatch1()
{
    mSfxMatch1.play(0.4);
}

void GameSounds::playSoundMatch2()
{
    mSfxMatch2.play(0.4);
}

void GameSounds::playSoundMatch3()
{
    mSfxMatch3.play(0.4);
}

// UI feedback. Both samples are peak-normalised to -3 dB, so the volumes here
// are the only place their relative loudness is set: hover fires constantly as
// the pointer sweeps the panel and has to stay under the music, while the click
// confirms an action and is allowed to cut through.
void GameSounds::playSoundButtonHover()
{
    mSfxButtonHover.play(0.2);
}

void GameSounds::playSoundButtonClick()
{
    mSfxButtonClick.play(0.35);
}

void GameSounds::playSoundError()
{
    // Deliberately the loudest effect in the game. All the samples sit at the
    // same -3 dB peak, so this number alone is what makes a rejected move cut
    // through the music and the match effects instead of being missed.
    mSfxError.play(0.5);
}
