/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Register access and the 1 MHz reference counter for the SL6801 drivers. */
#ifndef MMIO_SL6801_H
#define MMIO_SL6801_H

#include <stdbool.h>
#include <stdint.h>
#include "sl6801.h"

#define REG(a)   (*(volatile uint32_t *)(uintptr_t)(a))
#define REG16(a) (*(volatile uint16_t *)(uintptr_t)(a))
#define REG8(a)  (*(volatile uint8_t *)(uintptr_t)(a))

/* Microseconds on the 1 MHz reference counter (wraps every ~71 min: take
 * differences) */
static inline __attribute__((always_inline)) uint32_t sl6801_us(void)
{
    return REG(SL6801_COUNTER_LO);
}

/* Wait US microseconds: yields from a thread with interrupts on (playback
 * keeps running), spins otherwise; bounded if the counter stops */
void sl6801_udelay(uint32_t us);

/* The 64-bit counter (SL6801_COUNTER_HI:LO); false if the high word moved
 * between the reads (read again). Always inlined, so callers in SRAM
 * (.icode) do not call into flash. */
static inline __attribute__((always_inline))
bool sl6801_counter64(uint64_t *value)
{
    uint32_t high1 = REG(SL6801_COUNTER_HI);
    uint32_t low = REG(SL6801_COUNTER_LO);
    uint32_t high2 = REG(SL6801_COUNTER_HI);
    if (high1 != high2)
        return false;
    *value = ((uint64_t)high1 << 32) | low;
    return true;
}

#endif
