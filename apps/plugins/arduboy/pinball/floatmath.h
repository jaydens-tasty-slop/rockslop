/* The few libm calls Pinball makes, on the layer's small float helpers (a
 * plugin has no libm). */
#ifndef FLOATMATH_H
#define FLOATMATH_H

#include "../lib/ab_math.h"

static inline float pow(float x, int n)
{
  float r = 1.0f;
  while (n-- > 0) r *= x;
  return r;
}

static inline float round(float x) { return x < 0.0f ? -(float)(int)(-x + 0.5f) : (float)(int)(x + 0.5f); }

#endif
