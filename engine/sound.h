
#ifndef SOUND_H
#define SOUND_H

#include "audiere.h"

namespace Sound
{
    struct Exception{};

    void Init();
    void Shutdown();

    audiere::OutputStream* OpenSound(const char* fname);
}

#endif
