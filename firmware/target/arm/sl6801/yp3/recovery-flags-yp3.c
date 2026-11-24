/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Sets the PMU boot reason for a watchdog reset from the copied early entry
 * (recovery-entry-yp3.S), which owns a private stack in the copied prefix:
 * 7 (ROM USB update) in dev builds, 0 (boot the firmware) with
 * YP3_WATCHDOG_POWEROFF, whose next boot then powers off.
 * Runs before .data/.bss exist and before XIP: no globals, no calls outside
 * the prefix, IRQs masked. It talks to the PMU through the indexed bridge
 * (sl6801.h) and only sets flags; the watchdog
 * is programmed afterwards and a partial failure is handled by the caller.
 *
 * Boot reason as the bootloader reads it:
 *   (PMU[0x00] & 1) | (PMU[0x23] >> 6) << 1;  7 = ROM USB, 6 = SD update.
 * Reason 6 must never be requested, so PMU[0x00] bit 0 is set first. */
#include <stdint.h>
#include "config.h"
#include "cpu.h"
#include "pmu-yp3.h"
#include "recovery-yp3.h"

#define EARLY __attribute__((section(".init.yp3_early_recovery"), noinline))

/* PMU register values seen in normal boots (PMU23/00/20 = 00|c0 / 08|09 /
 * 2f; registers and bits in pmu-yp3.h). Anything else means an unknown
 * state, and arming is refused.
 * PMU[0x00] is PMU00_BIT3 alone or with PMU00_REASON_BIT0, PMU[0x23] is 0
 * or PMU23_REASON_MASK, and PMU[0x20] is PMU20_NORMAL. */
/* Word for the PMU emergency port (PMU_EMERGENCY, recovery-hw-yp3.S):
 * the working config writes ((PMU[0x20] | 0x80) << 8) | 0x20. Read as
 * value << 8 | PMU index; that layout is inferred, not documented. */
#define EMERGENCY_CMD(index, value) ((value) << 8 | (index))
/* PMU_D8 bit 1: cleared as in the validated recovery config; role
 * unknown. */
#define PMU_D8_BIT1       2u

/* Bridge transfer timing, as yp3dev does it: up to 64 status polls, 500 us
 * apart. The whole arming may read the 1 MHz counter
 * at most 65536 times and must finish within 1 s of the first read, which
 * also bounds a frozen counter. */
#define BRIDGE_POLLS      64
#define BRIDGE_POLL_US    500u
#define ARM_MAX_READS     65536u
#define ARM_MAX_US        1000000u

extern uint32_t yp3_early_recovery_read32(uint32_t);
extern void yp3_early_recovery_write32(uint32_t, uint32_t);

struct bounded_arm {
    uint32_t first_tick, remaining_reads;
    uint32_t *output;
};

static EARLY int sample(struct bounded_arm *s, uint32_t *now)
{
    if (!s->remaining_reads)
        return 0;
    --s->remaining_reads;
    *now = yp3_early_recovery_read32(SL6801_COUNTER_LO);
    /* Unsigned difference: one wrap of the low word is fine; a jump back or
     * more than ARM_MAX_US refuses before arming. remaining_reads bounds a
     * frozen or absent counter. */
    return (uint32_t)(*now - s->first_tick) <= ARM_MAX_US;
}

/* One PMU bridge read or write of register reg; 0 on any unexpected state. */
static EARLY int transfer(struct bounded_arm *s, uint32_t reg,
                          int writing, uint32_t *value)
{
    uint32_t command = (writing ? PMU_BRIDGE_CMD_WRITE : PMU_BRIDGE_CMD_READ) |
                       (reg << PMU_BRIDGE_CMD_INDEX_SHIFT);
    if (yp3_early_recovery_read32(PMU_BRIDGE_CMD) & PMU_BRIDGE_START)
        return 0;
    if (yp3_early_recovery_read32(PMU_BRIDGE_STATUS) & PMU_BRIDGE_STATUS_ERROR)
        return 0;
    if (writing)
        yp3_early_recovery_write32(PMU_BRIDGE_WDATA, *value);
    yp3_early_recovery_write32(PMU_BRIDGE_CMD, command);
    if (yp3_early_recovery_read32(PMU_BRIDGE_CMD) != command)
        return 0;
    if (writing)
        /* Set before START: even a failed transfer may have applied it. */
        s->output[EARLY_OUT_WRITE_TRIED] = 1;
    yp3_early_recovery_write32(PMU_BRIDGE_CMD, command | PMU_BRIDGE_START);
    for (unsigned polls = 0; polls < BRIDGE_POLLS; ++polls) {
        uint32_t began, now;
        if (!sample(s, &began))
            return 0;
        do {
            if (!sample(s, &now))
                return 0;
        } while ((uint32_t)(now - began) < BRIDGE_POLL_US);
        uint32_t observed = yp3_early_recovery_read32(PMU_BRIDGE_CMD);
        if (observed & PMU_BRIDGE_START)
            continue;
        if (observed != command)
            return 0;
        if (!writing)
            *value = yp3_early_recovery_read32(PMU_BRIDGE_RDATA) & 0xffu;
        return !(yp3_early_recovery_read32(PMU_BRIDGE_STATUS) &
                 PMU_BRIDGE_STATUS_ERROR);
    }
    return 0;
}

/* Set the watchdog-reset reason (file comment; reason 7: PMU[0x00] bit 0,
 * then PMU[0x23] bits 7:6), read back and verify each step, and fill
 * output[] (EARLY_OUT_* words, recovery-yp3.h) for the caller. Returns 1
 * when set. Uses no initialized or permanent RAM state. */
EARLY int yp3_early_recovery_arm_flags(uint32_t output[EARLY_OUT_WORDS])
{
    output[0] = output[1] = output[2] = output[3] = 0;
    struct bounded_arm s = {yp3_early_recovery_read32(SL6801_COUNTER_LO),
                            ARM_MAX_READS, output};
    uint32_t reason23 = 0, flags00 = 0, reset20 = 0, value;
    if (!transfer(&s, PMU_REG_REASON, 0, &reason23) ||
        !transfer(&s, PMU_REG_00, 0, &flags00) ||
        !transfer(&s, PMU_REG_RESET, 0, &reset20))
        return 0;
    uint32_t base00 = flags00;
    if ((reason23 != 0 && reason23 != PMU23_REASON_MASK) ||
        (base00 != PMU00_BIT3 &&
         base00 != (PMU00_BIT3 | PMU00_REASON_BIT0)) ||
        reset20 != PMU20_NORMAL)
        return 0;
    output[EARLY_OUT_RESET_BYTE] = reset20 | PMU20_RESET_REQUEST;
#ifdef YP3_WATCHDOG_POWEROFF
    /* Clear any reason left set, PMU[0x23] first (as
     * pmu_clear_recovery_reason in power-yp3.c), so a partial clear never leaves 6; each
     * write is read back. At most 7 transfers: a dev variant with 10 (one
     * more read of PMU[0x23]) failed arming at every boot (2026-10-23;
     * cause unconfirmed, likely the ARM_MAX_READS budget). */
    if (reason23) {
        value = 0;
        if (!transfer(&s, PMU_REG_REASON, 1, &value) ||
            !transfer(&s, PMU_REG_REASON, 0, &value) || value != 0)
            return 0;
    }
    if (flags00 & PMU00_REASON_BIT0) {
        value = flags00 & ~PMU00_REASON_BIT0;
        if (!transfer(&s, PMU_REG_00, 1, &value) ||
            !transfer(&s, PMU_REG_00, 0, &value) ||
            value != (flags00 & ~PMU00_REASON_BIT0))
            return 0;
    }
#else
    /* PMU[0x00] bit 0 first: if we stop half way, the reason is 1 or 7,
     * never 6 (SD update). */
    value = flags00 | PMU00_REASON_BIT0;
    if (!transfer(&s, PMU_REG_00, 1, &value) ||
        !transfer(&s, PMU_REG_00, 0, &value) ||
        value != (flags00 | PMU00_REASON_BIT0))
        return 0;
    value = reason23 | PMU23_REASON_MASK;
    if (!transfer(&s, PMU_REG_REASON, 1, &value) ||
        !transfer(&s, PMU_REG_REASON, 0, &value) ||
        value != (reason23 | PMU23_REASON_MASK) ||
        !transfer(&s, PMU_REG_00, 0, &value) ||
        value != (flags00 | PMU00_REASON_BIT0) ||
        !transfer(&s, PMU_REG_RESET, 0, &value) || value != reset20)
        return 0;
#endif
    output[EARLY_OUT_EMERGENCY_CMD] =
        EMERGENCY_CMD(PMU_REG_RESET, output[EARLY_OUT_RESET_BYTE]);
    output[EARLY_OUT_PMU_D8] =
        yp3_early_recovery_read32(PMU_D8) & ~PMU_D8_BIT1;
    return 1;
}
