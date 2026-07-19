<p align="center">
    <img src="https://raw.githubusercontent.com/JoseTomasTocino/freegemas/static/images/header_logo.png"><br>
    <img src="https://raw.githubusercontent.com/JoseTomasTocino/freegemas/static/images/header_gems.png"><br>
</p>

__Seajeweled__ is an open source version of the well known Bejeweled, for GNU/Linux, Windows and Mac. It's written in C++ using [SDL2](https://www.libsdl.org/). It is a fork of [Freegemas](https://github.com/JoseTomasTocino/freegemas) by José Tomás Tocino García. In the past it used Gosu instead of SDL2, and the old repository can be found at [Google Code](https://code.google.com/archive/p/freegemas/).

<p align="center">
    <img src="https://raw.githubusercontent.com/JoseTomasTocino/freegemas/static/images/screenshot_1.png">
</p>    
        


## Installation on Debian-based GNU/Linux systems

First, you need to install git and gcc:

    sudo apt-get install git build-essential cmake

Next, install SDL2 and JsonCpp from the repositories:

    sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libjsoncpp-dev
    
After that, clone the repo:

    git clone https://github.com/JoseTomasTocino/freegemas.git seajeweled
    
Do an out-of-source compilation and run the program:

    cd seajeweled
    mkdir build
    cd build
    cmake ..
    make
    ./seajeweled

## Installation on OS X

This assumes that you are already using [Homebrew](https://brew.sh/). You will need CMake and a few libraries to compile Seajeweled:

    brew install cmake sdl2 sdl2_mixer sdl2_ttf sdl2_image gettext jsoncpp

Now run the following commands to setup your environment to use Homebrew as a backup location for libraries.

```
[[ -z "${LIBRARY_PATH}" ]] && export LIBRARY_PATH=/usr/local/lib
export LIBRARY_PATH="$LIBRARY_PATH:$(brew --prefix)/lib"
```

After that, clone the repo:

    git clone https://github.com/JoseTomasTocino/freegemas.git seajeweled
    
Do an out-of-source compilation and run the program:

    cd seajeweled
    mkdir build
    cd build
    cmake ..
    make
    ./seajeweled

## Music licensing

The music in the game is [Night Prowler by Section 31 - Tech](https://opengameart.org/content/night-prowler),
licensed under [CC0](https://creativecommons.org/publicdomain/zero/1.0/).
