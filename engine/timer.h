#pragma once
#include "SDL/include/SDL.h"

// Ticks in a second.
const int timeRate = 100;

inline int GetTime() {
    return SDL_GetTicks() * timeRate / 1000;
}
