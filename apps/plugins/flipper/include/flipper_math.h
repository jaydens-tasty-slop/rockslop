/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Rockbox libc math.h has no prototypes; the layer provides a small libm
 * (flipper_core.c). furi.h includes this unless FLIPPER_NO_LIBM is defined
 * (games that declare their own atan2 etc.). */
#ifndef FLIPPER_MATH_H
#define FLIPPER_MATH_H
#ifdef __cplusplus
extern "C" {
#endif
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_E 2.7182818284590452354
double sin(double x);
double cos(double x);
double sqrt(double x);
double atan(double x);
double atan2(double y, double x);
double fabs(double x);
double floor(double x);
double ceil(double x);
double fmod(double a, double b);
float sinf(float x);
float cosf(float x);
float sqrtf(float x);
float fabsf(float x);
float atanf(float x);
float infinityf(void);
float roundf(float x);
float floorf(float x);
float ceilf(float x);
double fmax(double a, double b);
double fmin(double a, double b);
float fmaxf(float a, float b);
float fminf(float a, float b);
#ifdef __cplusplus
}
#endif
#endif
