#include "halton.h"


float halton(int index, int base) {
    float result = 0.0f;
    float f = 1.0f / base;
    int i = index;
    while (i > 0) {
        result += f * (i % base);
        i = i / base;
        f = f / base;
    }
    return result;
}

gmtl::Point2f halton2D(int index, int base1, int base2) {
    return gmtl::Point2f(halton(index, base1), halton(index, base2));
}