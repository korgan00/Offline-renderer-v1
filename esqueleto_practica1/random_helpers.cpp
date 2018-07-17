#include "random_helpers.h"
#include <core.h>

int randomInt(int max) {
    return rand() % max;
}

float randomFloat01() {
    float n = static_cast<float>(rand());
    return n / static_cast<float>(RAND_MAX);
}

gmtl::Point3f randomPoint3f() {
    gmtl::Point2f r = halton2D(randomInt(10000), HALTON_SEED_X, HALTON_SEED_Y);

    float r1 = r[0];
    float r2 = (r[1] * 2) - 1;
    float phi = 2 * M_PI * r1;
    float m = sqrt(1.0f - (r2 * r2));

    return gmtl::Point3f(cos(phi) * m, sin(phi) * m, r2);
}

gmtl::Vec3f randomVec3f() {
    gmtl::Point2f r = halton2D(randomInt(10000), HALTON_SEED_X, HALTON_SEED_Y);

    float r1 = r[0];
    float r2 = (r[1] * 2) - 1;
    float phi = 2 * M_PI * r1;
    float m = sqrt(1.0f - (r2 * r2));

    return gmtl::Vec3f(cos(phi) * m, sin(phi) * m, r2);
}

bool russianRoulletteCheck() {
    return randomFloat01() <= RUSSIAN_ROULLETTE_PROC;
}