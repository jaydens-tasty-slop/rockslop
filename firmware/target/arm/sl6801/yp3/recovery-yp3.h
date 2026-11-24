/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef RECOVERY_YP3_H
#define RECOVERY_YP3_H

#include "config.h"
#include "cpu.h"

/* Recovery watchdog (build switch YP3_RECOVERY_WATCHDOG): the copied early
 * entry arms a ~16 s watchdog before XIP setup; the kernel tick feeds it.
 * Dev builds also arm PMU reason 7, so parking without feeds returns the
 * player to ROM USB; with YP3_WATCHDOG_POWEROFF (release) the reset boots
 * the firmware, which powers off at once.
 * A plain `b .` park is not reliable for the expiry: keep reading a bus
 * register (the 1 MHz counter) while waiting. Also used from assembly. */

/* out[] words that yp3_early_recovery_arm_flags() (recovery-flags-yp3.c)
 * fills for the early entry (recovery-entry-yp3.S), which keeps them on its
 * temporary stack. Indices of 32-bit words. */
#define EARLY_OUT_EMERGENCY_CMD 0   /* word for PMU_EMERGENCY */
#define EARLY_OUT_PMU_D8        1   /* current PMU_D8 with bit 1 clear */
#define EARLY_OUT_RESET_BYTE    2   /* PMU[0x20] | 0x80: reset request */
#define EARLY_OUT_WRITE_TRIED   3   /* nonzero once a PMU write may have
                                       landed */
#define EARLY_OUT_WORDS         4

#ifdef __ASSEMBLER__
    .macro yp3_recovery_wait
    ldr r0, =SL6801_COUNTER_LO
.Lyp3_recovery_wait\@:
    ldr r1, [r0]
    b .Lyp3_recovery_wait\@
    .endm
#else
#include <stdint.h>

/* Program the watchdog (mode 0x2001, selector 0x71, calibration word
 * 0x7fff00: ~16 s) and the PMU emergency command (recovery-hw-yp3.S).
 * Called from the copied SRAM early entry only. */
void yp3_recovery_watchdog_prepare(uint32_t calibration,
                                  uint32_t emergency_command, uint32_t pmu_d8);
/* Start the 1 Hz kernel-tick feed (recovery-watchdog-yp3.c). */
void yp3_recovery_watchdog_start(void);
/* Watchdog feeds also require this thread to run (power_init). */
void yp3_recovery_heartbeat_start(void);
/* One feed: write the refresh key 0x28000001 to watchdog register
 * 0x40083010 (recovery-hw-yp3.S). */
void yp3_recovery_watchdog_refresh(void);
/* Stop the watchdog (selector 0): power-off only. */
void yp3_recovery_watchdog_stop(void);
#ifdef YP3_WATCHDOG_POWEROFF
/* First thing in system_init(): power off if this boot is a watchdog reset
 * of a running firmware, else mark the firmware running. */
void yp3_recovery_check_watchdog_reset(void);
/* Deliberate exit (reboot, power-off): the next boot is not a hang. */
void yp3_recovery_clear_run_marker(void);
/* Power-off after a watchdog reset (power-yp3.c). */
void yp3_power_off_after_hang(void) __attribute__((noreturn));
#endif
/* Park with IRQs masked, running from SRAM (system-sl6801.c). */
void yp3_recovery_park(void) __attribute__((noreturn));
/* USB pull-up state kept by the USB driver; dropped before a recovery
 * watchdog reset so the host notices the device leaving. */
extern volatile uint8_t yp3_usb_softconn;
void yp3_usb_drop_softconn(void);

static inline __attribute__((always_inline, noreturn))
void yp3_recovery_wait(void)
{
    for (;;)
        __asm__ volatile("ldr r1, [%0]" :: "l"((uintptr_t)SL6801_COUNTER_LO)
                         : "r1", "memory");
}
#endif
#endif
