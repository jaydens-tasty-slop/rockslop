/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Small float trig for games that call sin/cos/atan2 (no libm on this target).
 * Accuracy ~1e-3, plenty for sprite movement. A game opts in with
 * #include "../lib/ab_math.h". */
#ifndef AB_MATH_H
#define AB_MATH_H

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

static inline float ab_sin(float x)
{
    const float twopi = 2.0f * (float)PI;
    while (x > (float)PI) x -= twopi;
    while (x < -(float)PI) x += twopi;
    float y = 1.27323954f * x - 0.405284735f * x * (x < 0 ? -x : x);
    return 0.225f * (y * (y < 0 ? -y : y) - y) + y;
}

static inline float ab_cos(float x) { return ab_sin(x + (float)PI / 2.0f); }

static inline float ab_atan2(float y, float x)
{
    float ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    if (ax == 0 && ay == 0) return 0;
    float a = (ax < ay ? ax : ay) / (ax > ay ? ax : ay);
    float s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = (float)PI / 2.0f - r;
    if (x < 0) r = (float)PI - r;
    return y < 0 ? -r : r;
}

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif

static inline float ab_sqrt(float x)
{
    if (x <= 0) return 0;
    float r = x > 1 ? x / 2 : 1;
    for (int i = 0; i < 12; i++) r = 0.5f * (r + x / r);
    return r;
}

static inline float ab_floor(float x)
{
  int i = (int)x;
  return (float)(x < (float)i ? i - 1 : i);
}

static inline float ab_ceil(float x)
{
  int i = (int)x;
  return (float)(x > (float)i ? i + 1 : i);
}

#define ceil(x) ab_ceil(x)
#define ceilf(x) ab_ceil(x)
#define floor(x) ab_floor(x)
#define floorf(x) ab_floor(x)
#define sin(x) ab_sin(x)
#define cos(x) ab_cos(x)
#define atan2(y, x) ab_atan2(y, x)
#define sqrt(x) ab_sqrt(x)
#define sinf(x) ab_sin(x)
#define cosf(x) ab_cos(x)
#define atan2f(y, x) ab_atan2(y, x)
#define sqrtf(x) ab_sqrt(x)

#endif
