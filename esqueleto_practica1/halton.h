#pragma once
#include <gmtl/gmtl.h>

float halton(int index, int base);
gmtl::Point2f halton2D(int index, int base1, int base2);