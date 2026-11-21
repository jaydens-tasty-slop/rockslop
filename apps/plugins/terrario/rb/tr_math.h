/* The few libm functions Terrario uses, without libm. */
#pragma once
#include <stdint.h>

/* Single precision when the game runs on the FPU (lib/plugin_fpu.h). */
#ifdef PLUGIN_FPU
typedef float tr_real;
#else
typedef double tr_real;
#endif

#define TR_PI 3.14159265358979323846

static inline tr_real tr_floor(tr_real x)
{
  long long i = (long long)x;
  return (tr_real)(i > x ? i - 1 : i);
}
static inline tr_real ceil(tr_real x) { return -tr_floor(-x); }
static inline tr_real round(tr_real x) { return x < 0 ? -tr_floor(-x + 0.5) : tr_floor(x + 0.5); }
static inline float roundf(float x) { return (float)round(x); }
static inline float fabsf(float x) { return x < 0 ? -x : x; }

static inline tr_real tr_cos(tr_real x)
{
  /* reduce to [-pi, pi] and use a Taylor series */
  x -= tr_floor(x / (2 * TR_PI) + 0.5) * 2 * TR_PI;
  tr_real x2 = x * x, t = 1, s = 1;
  for (int i = 1; i <= 10; i++)
  {
    t *= -x2 / ((2 * i - 1) * (2 * i));
    s += t;
  }
  return s;
}
static inline tr_real cos(tr_real x) { return tr_cos(x); }
static inline tr_real sin(tr_real x) { return tr_cos(x - TR_PI / 2); }
static inline float cosf(float x) { return (float)tr_cos(x); }

static inline tr_real tr_exp(tr_real x)
{
  int n = 0;
  while (x > 0.5) { x /= 2; n++; }
  while (x < -0.5) { x /= 2; n++; }
  tr_real t = 1, s = 1;
  for (int i = 1; i <= 12; i++)
  {
    t *= x / i;
    s += t;
  }
  while (n--)
    s *= s;
  return s;
}

static inline tr_real tr_log(tr_real x)
{
  /* ln x = 2 atanh((x-1)/(x+1)) after scaling x into [0.5, 2) */
  int k = 0;
  while (x > 2) { x /= 2; k++; }
  while (x < 0.5) { x *= 2; k--; }
  tr_real y = (x - 1) / (x + 1), y2 = y * y, t = y, s = 0;
  for (int i = 1; i < 30; i += 2)
  {
    s += t / i;
    t *= y2;
  }
  return 2 * s + k * 0.693147180559945;
}

static inline tr_real pow(tr_real x, tr_real y)
{
  if (y == (tr_real)(int)y)
  {
    int n = (int)y;
    tr_real r = 1;
    for (int i = 0; i < (n < 0 ? -n : n); i++)
      r *= x;
    return n < 0 ? 1 / r : r;
  }
  return tr_exp(y * tr_log(x));
}

static inline tr_real modf(tr_real x, tr_real *ip)
{
  tr_real i = x < 0 ? -tr_floor(-x) : tr_floor(x);
  *ip = i;
  return x - i;
}
