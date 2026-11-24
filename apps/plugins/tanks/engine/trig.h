#ifndef TANKS_TRIG_H
#define TANKS_TRIG_H

#include "util.h"

typedef uint32_t angle_t; /* 2^32 is a full circle */

/* Converts a constant integer number of degrees to an angle_t */
#define DEGREES_TO_ANGLE(deg) ((angle_t)(int64_t)((double)(deg) * (4294967296.0 / 360.0)))



int fast_sin(angle_t angle);
#define fast_cos(rot) fast_sin(DEGREES_TO_ANGLE(90) - (rot))

int fast_sec(angle_t angle);
#define fast_csc(rot) fast_sec(DEGREES_TO_ANGLE(90) - (rot))

#define TRIG_SCALE 256
#define TRIG_PRECISION_BITS 8
#define TRIG_PRECISION (1 << TRIG_PRECISION_BITS)

uint fast_atan2(int y, int x);

#endif //TANKS_TRIG_H
