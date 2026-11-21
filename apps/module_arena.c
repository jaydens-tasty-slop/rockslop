/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "module_arena.h"
/* RING: the PCM ring in the arena's top (TARGET_ARENA_PCM_SIZE); codecs may
 * run beside it, plugins need the whole arena */
enum { FREE = 0, CODEC = 1, PLUGIN = 2, PLUGIN_PENDING = 4, RING = 8 };
static unsigned int state;

/* Set BIT unless any of BLOCKERS is set */
static bool acquire(unsigned int bit, unsigned int blockers)
{
    unsigned int old = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
    do {
        if (old & blockers)
            return false;
    } while (!__atomic_compare_exchange_n(&state, &old, old | bit, false,
                                         __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE));
    return true;
}

unsigned int module_arena_state(void)
{
    return __atomic_load_n(&state, __ATOMIC_ACQUIRE);
}

bool module_arena_acquire_codec(void)
{
    return acquire(CODEC, CODEC | PLUGIN | PLUGIN_PENDING);
}
void module_arena_release_codec(void)
{
    __atomic_fetch_and(&state, ~CODEC, __ATOMIC_RELEASE);
}
bool module_arena_codec_active(void)
{
    return __atomic_load_n(&state, __ATOMIC_ACQUIRE) & CODEC;
}
bool module_arena_request_plugin(void)
{
    return acquire(PLUGIN_PENDING, PLUGIN | PLUGIN_PENDING);
}
bool module_arena_acquire_plugin(void)
{
    unsigned int old = PLUGIN_PENDING;
    return __atomic_compare_exchange_n(&state, &old, PLUGIN, false,
                                      __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}
void module_arena_release_plugin(void)
{
    __atomic_fetch_and(&state, ~(PLUGIN | PLUGIN_PENDING), __ATOMIC_RELEASE);
}
#ifdef TARGET_ARENA_PCM_SIZE
bool module_arena_acquire_ring(void)
{
    return acquire(RING, RING | PLUGIN | PLUGIN_PENDING);
}
void module_arena_release_ring(void)
{
    __atomic_fetch_and(&state, ~RING, __ATOMIC_RELEASE);
}
#endif
