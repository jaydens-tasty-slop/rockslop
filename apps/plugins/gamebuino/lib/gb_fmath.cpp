/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Single precision libm pieces the games use, in their own file so they
 * are built for the FPU with the games (gb.cpp holds the sound synth that
 * runs in interrupt context and stays soft-float). */
#include "Gamebuino-Meta.h"

#ifndef GB_HOST_MATH
extern "C" float sqrtf(float x)
{
#ifdef PLUGIN_FPU
    float r;
    if (x <= 0)
        return 0;
    __asm__("vsqrt.f32 %0, %1" : "=t"(r) : "t"(x));   /* builtin would call sqrtf for errno */
    return r;
#else
    if (x <= 0)
        return 0;
    uint32_t i;
    memcpy(&i, &x, 4);
    i = 0x1fbb4f2e + (i >> 1);      /* first guess from the exponent */
    float r;
    memcpy(&r, &i, 4);
    for (int k = 0; k < 4; k++)
        r = 0.5f * (r + x / r);
    return r;
#endif
}

extern "C" float tanf(float x)
{
    /* |x| <= ~1: odd Taylor series, enough for a projection matrix */
    float x2 = x * x;
    return x * (1 + x2 * (1.f / 3 + x2 * (2.f / 15 + x2 * (17.f / 315 + x2 * 62.f / 2835))));
}

/* atan2 to about 0.3 degrees: atan(z) ~ z (0.9724 - 0.1919 z^2) for |z| <= 1 */
extern "C" float atan2f(float y, float x)
{
    float ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    if (ax == 0 && ay == 0)
        return 0;
    bool swap = ay > ax;
    float z = swap ? ax / ay : ay / ax;
    float a = z * (0.9724f - 0.1919f * z * z);
    if (swap)
        a = 1.5707963f - a;
    if (x < 0)
        a = 3.1415927f - a;
    return y < 0 ? -a : a;
}

extern "C" double atan2(double y, double x)
{
    return atan2f((float)y, (float)x);
}

/* sin/cos to about 1e-4: reduce to [-pi, pi], then a parabola with one
 * correction pass (Bhaskara-style, as used by many small-MCU codebases) */
extern "C" float sinf(float x)
{
    const float pi = 3.14159265f;
    x -= 6.28318531f * (int)(x / 6.28318531f);
    if (x > pi)
        x -= 2 * pi;
    else if (x < -pi)
        x += 2 * pi;
    float y = x * (1.27323954f - 0.405284735f * (x < 0 ? -x : x));
    return 0.225f * (y * (y < 0 ? -y : y) - y) + y;
}

extern "C" float cosf(float x)
{
    return sinf(x + 1.57079633f);
}

extern "C" double sin(double x) { return sinf((float)x); }
extern "C" double cos(double x) { return cosf((float)x); }
extern "C" double sqrt(double x) { return sqrtf((float)x); }
#endif
