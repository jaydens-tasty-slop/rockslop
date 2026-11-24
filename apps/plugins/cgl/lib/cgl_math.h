/* Minimal single-precision math for Crisp Game Lib on Rockbox (no libm). */
#ifndef CGL_MATH_H
#define CGL_MATH_H

#include "plugin.h"

#undef M_PI
#define M_PI 3.14159265f

float floorf(float x);
float ceilf(float x);
float fabsf(float x);
float fmaxf(float a, float b);
float fminf(float a, float b);
float fmodf(float x, float y);
float sqrtf(float x);
float sinf(float x);
float cosf(float x);
float atan2f(float y, float x);

#define floor floorf
#define ceil ceilf
#define fabs fabsf
#define sqrt sqrtf
#define sin sinf

#endif
