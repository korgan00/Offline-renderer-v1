#pragma once
#include "halton.h"

#ifndef RUSSIAN_ROULLETTE_PROC
    #define RUSSIAN_ROULLETTE_PROC 0.2f
#endif // !RUSSIAN_ROULLETTE_PROC
#ifndef HALTON_SEED_X
    #define HALTON_SEED_X 3
#endif // !HALTON_SEED_X
#ifndef HALTON_SEED_Y
    #define HALTON_SEED_Y 5
#endif // !HALTON_SEED_Y

int randomInt(int max);
float randomFloat01();
gmtl::Point3f randomPoint3f();
gmtl::Vec3f randomVec3f();
bool russianRoulletteCheck();