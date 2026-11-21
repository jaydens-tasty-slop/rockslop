#include <limits.h>
#include "trig.h"

#include "trig_tables.h"

int fast_sin(angle_t angle) {
    angle >>= INT_BITS - TRIG_PRECISION_BITS;
    if(angle < TRIG_PRECISION / 4) return sin_table[angle];
    if(angle == TRIG_PRECISION / 4) return TRIG_SCALE;
    if(angle < TRIG_PRECISION / 2) return sin_table[TRIG_PRECISION / 2 - angle];
    if(angle < TRIG_PRECISION * 3 / 4) return -sin_table[angle - TRIG_PRECISION / 2];
    if(angle == TRIG_PRECISION * 3 / 4) return -TRIG_SCALE;
    return -sin_table[TRIG_PRECISION - angle];
}

int fast_sec(angle_t angle) {
    angle >>= INT_BITS - TRIG_PRECISION_BITS;
    if(angle < TRIG_PRECISION / 4) return sec_table[angle];
    if(angle == TRIG_PRECISION / 4) return INT_MAX;
    if(angle < TRIG_PRECISION / 2) return -sec_table[TRIG_PRECISION / 2 - angle];
    if(angle < TRIG_PRECISION * 3 / 4) return -sec_table[angle - TRIG_PRECISION / 2];
    if(angle == TRIG_PRECISION * 3 / 4) return INT_MIN;
    return sec_table[TRIG_PRECISION - angle];
}

/* fast_atan2() works with a 24 bit circle (2^24 = 360 degrees), as on the
 * original eZ80, and widens the result. */
#define Q_FROM_FLOAT(f) ((int)((f) * (double)(1l << 23l)))
#define M_1_PI 0.31830988618379067154
#define DEG24(deg) ((int)((deg) * (double)(1l << 24) / 360))

inline int qmul(int a, int b) {
    return (a >> 12) * (b >> 11);
}

inline int qdiv(int a, int b) {
    int a1 = a * (1 << 7);
    int d = (a1 / b);
    return d * (1 << 16);
}

inline int nabs(int x) {
    if(x > 0) x = -x;
    return x;
}

// Credit: https://geekshavefeelings.com/posts/fixed-point-atan2
static uint atan2_24(int y, int x) {
    if(x == y) { // x/y or y/x would return -1 since 1 isn't representable
        if(y > 0) { // 1/8
            return DEG24(360.0 / 8);
        } else if(y < 0) { // 5/8
            return DEG24(360.0 * 5 / 8);
        } else { // x = y = 0
            return 0;
        }
    }
    int nabs_y = nabs(y);
    int nabs_x = nabs(x);
    if(nabs_x < nabs_y) { // octants 1, 4, 5, 8
        int y_over_x = qdiv(y, x);
        int correction = qmul(Q_FROM_FLOAT(0.273 * M_1_PI), nabs(y_over_x));
        int unrotated = qmul(Q_FROM_FLOAT(0.25 + 0.273 * M_1_PI) + correction, y_over_x);
        if(x > 0) { // octants 1, 8
            return unrotated;
        } else { // octants 4, 5
            return DEG24(180) + unrotated;
        }
    } else { // octants 2, 3, 6, 7
        int x_over_y = qdiv(x, y);
        int correction = qmul(Q_FROM_FLOAT(0.273 * M_1_PI), nabs(x_over_y));
        int unrotated = qmul(Q_FROM_FLOAT(0.25 + 0.273 * M_1_PI) + correction, x_over_y);
        if(y > 0) { // octants 2, 3
            return DEG24(90) - unrotated;
        } else { // octants 6, 7
            return DEG24(270) - unrotated;
        }
    }
}

uint fast_atan2(int y, int x) {
    return atan2_24(y, x) << 8;
}
