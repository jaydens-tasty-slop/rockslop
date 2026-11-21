/* Small helpers for the Tanks! engine (MIT, see ../LICENSE). Replaces the
 * eZ80-specific util.h of the original. */
#ifndef H_UTIL
#define H_UTIL

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef unsigned int uint;
typedef unsigned int uint24_t; /* the original ran with 24 bit integers */

#define INT_BITS 32 /* angles are full-range 32 bit values */

enum {
    AXIS_X,
    AXIS_Y
};

#ifdef TANKS_HOST
#include <stdlib.h>
#else
static inline int abs(int x) { return x < 0 ? (int)(0u - (unsigned)x) : x; }
#endif

/* The CE toolchain's randInt(): uniform in [min, max]. Seeded by the front end. */
extern "C" unsigned tanks_random(void);
static inline unsigned randInt(unsigned min, unsigned max)
{
    return min + tanks_random() % (max - min + 1);
}

/* operator new without libstdc++: pool allocator in rockbox/pool.cpp */
#ifndef TANKS_HOST
namespace std {
struct nothrow_t { explicit nothrow_t() = default; };
extern const nothrow_t nothrow;
}
void *operator new(size_t, const std::nothrow_t &) noexcept;
#else
#include <new>
#endif

#endif
