/* Single-precision math helpers; accuracy is ample for pixel-sized games. */
#include "../lib/cgl_math.h"
#include <stdint.h>

#undef floor
#undef ceil
#undef fabs
#undef sqrt
#undef sin

float fabsf(float x) { return x < 0 ? -x : x; }
float fmaxf(float a, float b) { return a > b ? a : b; }
float fminf(float a, float b) { return a < b ? a : b; }

static float truncf_(float x)
{
    if (x >= 2147483648.0f || x <= -2147483648.0f)
        return x;
    return (float)(int32_t)x;
}

float floorf(float x)
{
    float t = truncf_(x);
    return t > x ? t - 1.0f : t;
}

float ceilf(float x)
{
    float t = truncf_(x);
    return t < x ? t + 1.0f : t;
}

float fmodf(float x, float y)
{
    if (y == 0.0f)
        return 0.0f;
    return x - truncf_(x / y) * y;
}

float sqrtf(float x)
{
    union { float f; uint32_t u; } v = { x };
    if (x <= 0.0f)
        return 0.0f;
    v.u = (v.u >> 1) + 0x1fbd1df5;
    float r = v.f;
    r = 0.5f * (r + x / r);
    r = 0.5f * (r + x / r);
    r = 0.5f * (r + x / r);
    return r;
}

float sinf(float x)
{
    const float pi = 3.14159265f;
    x = fmodf(x, 2 * pi);
    if (x > pi)
        x -= 2 * pi;
    else if (x < -pi)
        x += 2 * pi;
    /* Bhaskara-style parabola with a precision correction. */
    float y = (4.0f / pi) * x - (4.0f / (pi * pi)) * x * fabsf(x);
    return 0.225f * (y * fabsf(y) - y) + y;
}

float cosf(float x)
{
    return sinf(x + 1.57079633f);
}

/* atan(r) for 0 <= r <= 1, max error ~1e-5 rad. */
static float atan_unit(float r)
{
    float r2 = r * r;
    return r * (0.9998660f + r2 * (-0.3302995f + r2 * (0.1801410f +
                r2 * (-0.0851330f + r2 * 0.0208351f))));
}

float atan2f(float y, float x)
{
    const float pi = 3.14159265f;
    float ax = fabsf(x), ay = fabsf(y), a;
    if (ax == 0.0f && ay == 0.0f)
        return 0.0f;
    if (ax >= ay)
        a = atan_unit(ay / ax);
    else
        a = 1.57079633f - atan_unit(ax / ay);
    if (x < 0)
        a = pi - a;
    return y < 0 ? -a : a;
}

char *strncat_(char *dst, const char *src, size_t n)
{
    char *d = dst + rb->strlen(dst);
    while (n-- && *src)
        *d++ = *src++;
    *d = '\0';
    return dst;
}
