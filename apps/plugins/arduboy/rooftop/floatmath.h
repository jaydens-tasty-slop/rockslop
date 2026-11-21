/* The few libm calls Rooftop makes, on the layer's small float helpers (a
 * plugin has no libm). */
#ifndef FLOATMATH_H
#define FLOATMATH_H

#include "../lib/ab_math.h"

static inline float fabs(float x) { return x < 0.0f ? -x : x; }

static inline float round(float x) { return x < 0.0f ? -(float)(int)(-x + 0.5f) : (float)(int)(x + 0.5f); }

#endif
