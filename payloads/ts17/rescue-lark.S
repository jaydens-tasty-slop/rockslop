/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Rescue stub: the image's first sector (LARK_RESCUE_SIZE), ahead of
 * Rockbox. The boot chain's recovery image jumps here (vector word 1) as it
 * did into stock's application.
 *
 * FROZEN (v2, 2026-11-14): never to change. These bytes are on players
 * and pinned by sha256 in rockslop (the flasher, which writes its own copy
 * where there is no stub) and in auto-rockbox; ts17dev/build.sh fails a
 * build whose stub differs. Release packages leave this sector out (they
 * start at NOR 0x15000), and nothing rewrites it on an update, so USB
 * download mode stays within reach whatever is flashed behind it:
 *
 * - the boot mode word and the watchdog (~11 s, never fed here) come first:
 *   from then on any hang, fault or bad jump resets into download mode;
 * - USB power in and the on/off key or the pad's Down held for
 *   RESCUE_HOLD_STEPS checks 10 ms apart: download mode now. Plugging USB
 *   in starts a player that is off, so Down held while plugging in is
 *   enough;
 * - Rockbox's vector table implausible (erased or half-written flash, an
 *   ELF file, the wrong link address): download mode now with USB power
 *   in. On battery, power off instead: download mode has no timeout and
 *   would sit there unseen, screen off, until the cell is flat. Plugging
 *   USB in starts it again, into download mode;
 * - otherwise Rockbox's reset vector, the boot mode word still set (crt0
 *   sets it again and keeps it).
 *
 * Entered from the boot chain (PRIMASK set, its own stack, a stale VTOR,
 * 96 MHz) or from download mode by the host tools. It uses no stack and
 * takes no exception, writes nothing but the registers below, and leaves r0
 * to Rockbox. Its bytes depend on these constants only, never on the rest
 * of the build, so every build carries the same stub.
 */
#include "config.h"
#include "cpu.h"

#define BOOT_MODE       0x4000c03c      /* LARK_BOOT_MODE */
#define BOOT_ADFU       0x42520100      /* LARK_BOOT_ADFU */
#define WDT_CTL         0x4000c020      /* LARK_WDT_CTL */
#define WDT_ARM         0x1d            /* enable, period code 6 (~11 s), feed */
#define WDT_NOW         0x1e            /* enable, period code 7: reset now */
#define PMU_DET         0x40004010      /* LARK_PMU_DET */
#define PMU_DET_DC5V    (1 << 0)
#define PMU_KEY         0x40004108      /* LARK_PMU_KEY */
#define PMU_KEY_ONOFF   (1 << 17)
#define PMU_OFF         0x4000410c      /* LARK_PMU_OFF: any write */
#define PMU_WAKE        0x40004110      /* LARK_PMU_WAKE */
#define PMU_WAKE_ON     0x115           /* WAKE_BASE | WAKE_DC5V, as power_off() */
#define OFF_SETTLE      50000           /* loops, >= 0.5 ms: wake, then off */
#define PMUADC_CTL      0x40004200      /* LARK_PMUADC_CTL */
#define PMUADC_LADDER   ((1 << 5) | (1 << 23)) /* the key ladder's channel, run */
#define PMUADC_KEYS     0x40004224      /* LARK_PMUADC_DATA(LARK_PMUADC_KEYS) */
#define LADDER_DOWN     0x1d00          /* to 0x2500: button-ts17.c's Down */
#define LADDER_DOWN_SPAN 0x0800
#define SYST_CSR        0xe000e010      /* RVR +4, CVR +8 */
#define SYST_COUNTFLAG  (1 << 16)
#define SYST_10MS       (96000000 / 100) /* the boot chain's 96 MHz */

/* ~3.5 s from power-on in all: the boot chain takes ~2 s, and the PMU
 * resets the player at ~7 s. A power-on press is let go long before. */
#define RESCUE_HOLD_STEPS 150
#define RESCUE_VERSION  2

#define APP_BASE        (LARK_XIP_BASE + LARK_RESCUE_SIZE)

    .syntax unified
    .thumb
    .section .rescue,"ax",%progbits

    .global rescue_header
rescue_header:
    .word   LARK_RAM0_BASE + LARK_RAM0_SIZE @ stack pointer: unused
    .word   rescue_reset                    @ the boot chain's entry
    .ascii  "RBRS"                          @ the host tools' mark
    .word   RESCUE_VERSION

    .thumb_func
rescue_reset:
    cpsid   i
    ldr     r1, =BOOT_MODE
    ldr     r2, =BOOT_ADFU
    str     r2, [r1]
    ldr     r1, =WDT_CTL
    movs    r2, #WDT_ARM
    str     r2, [r1]

    ldr     r1, =PMU_DET                    @ no USB power: on at once
    ldr     r2, [r1]
    tst     r2, #PMU_DET_DC5V
    beq     app
    ldr     r1, =PMUADC_CTL                 @ the boot ROM leaves the ladder
    ldr     r2, [r1]                        @ sampled; make sure, and give it
    ldr     r3, =PMUADC_LADDER              @ a period for a fresh sample
    orrs    r2, r2, r3
    str     r2, [r1]
    ldr     r5, =SYST_CSR
    ldr     r2, =SYST_10MS - 1
    str     r2, [r5, #4]
    movs    r2, #0
    str     r2, [r5, #8]
    movs    r2, #5                          @ processor clock, enable
    str     r2, [r5]
    movs    r4, #RESCUE_HOLD_STEPS
    b       wait
hold:
    ldr     r1, =PMU_DET
    ldr     r2, [r1]
    tst     r2, #PMU_DET_DC5V
    beq     app
    ldr     r1, =PMU_KEY
    ldr     r2, [r1]
    tst     r2, #PMU_KEY_ONOFF
    bne     held
    ldr     r1, =PMUADC_KEYS
    ldr     r2, [r1]
    ubfx    r2, r2, #0, #14
    subs    r2, r2, #LADDER_DOWN
    cmp     r2, #LADDER_DOWN_SPAN
    bhs     app
held:
    subs    r4, r4, #1
    beq     adfu
wait:
    ldr     r2, [r5]                        @ reading clears COUNTFLAG
    tst     r2, #SYST_COUNTFLAG
    beq     wait
    b       hold

app:
    ldr     r1, =APP_BASE
    ldr     r2, [r1]                        @ stack pointer: in RAM0
    ldr     r3, =LARK_RAM0_BASE + 1
    subs    r2, r2, r3
    ldr     r3, =LARK_RAM0_SIZE
    cmp     r2, r3
    bhs     bad
    ldr     r2, [r1, #4]                    @ reset vector: Thumb, in the image
    tst     r2, #1
    beq     bad
    subs    r3, r2, r1
    ldr     r6, =LARK_XIP_SIZE - LARK_RESCUE_SIZE
    cmp     r3, r6
    bhs     bad
    bx      r2

bad:                                        @ no image to run
    ldr     r1, =PMU_DET
    ldr     r2, [r1]
    tst     r2, #PMU_DET_DC5V
    bne     adfu
    ldr     r1, =PMU_KEY                    @ a held key would wake it again
release:
    ldr     r2, [r1]
    tst     r2, #PMU_KEY_ONOFF
    bne     release
    ldr     r1, =PMU_WAKE
    ldr     r2, =PMU_WAKE_ON
    str     r2, [r1]
    ldr     r3, =OFF_SETTLE
settle:
    subs    r3, r3, #1
    bne     settle
    ldr     r1, =PMU_OFF
    str     r3, [r1]
    b       .                               @ still on: the watchdog's reset

adfu:
    ldr     r1, =WDT_CTL
    movs    r2, #WDT_NOW
    str     r2, [r1]
    b       .

    .ltorg
