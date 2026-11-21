/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Recovery watchdog programming. The sequence and values are a working
 * recovery configuration, validated on the hardware: with PMU reason 7
 * armed, the watchdog resets the SoC ~16 s after the last feed and the ROM
 * then enters USB recovery. Register roles are known only as far as this
 * sequence establishes them. */
#include "config.h"
#include "cpu.h"
    .syntax unified
    .cpu cortex-m4
    .thumb

/* The watchdog block's registers are in sl6801.h. */

    .section .init.yp3_recovery_tail,"ax",%progbits

/* r0 = WDT_CAL value, r1 = emergency port word, r2 = PMU_D8 value (bit 1
 * already cleared). Runs from the copied prefix with IRQs masked. */
    .global yp3_recovery_watchdog_prepare
    .type yp3_recovery_watchdog_prepare,%function
    .thumb_func
yp3_recovery_watchdog_prepare:
    push {r4,r5}
    ldr r4,=PMU_EMERGENCY
    str r1,[r4]
    ldr r4,=PMU_D8
    str r2,[r4]
    ldr r4,=WDT_BASE
    movs r5,#0
    str r5,[r4,#WDT_SELECTOR]   /* stop it, then reconfigure */
    str r5,[r4,#WDT_CTRL]
    movs r5,#1
    str r5,[r4,#WDT_ACK]
    str r5,[r4,#WDT_CTRL]
    movw r5,#WDT_MODE_RESET
    str r5,[r4,#WDT_MODE]
    movs r5,#WDT_SELECTOR_16S
    str r5,[r4,#WDT_SELECTOR]
    str r0,[r4,#WDT_CAL]
    ldr r5,=WDT_REFRESH_KEY
    str r5,[r4,#WDT_REFRESH]    /* first feed starts the 16 s period */
    dsb
    pop {r4,r5}
    bx lr
    .ltorg
    .size yp3_recovery_watchdog_prepare,.-yp3_recovery_watchdog_prepare

    .section .text.yp3_recovery_refresh,"ax",%progbits
    .global yp3_recovery_watchdog_refresh
    .type yp3_recovery_watchdog_refresh,%function
    .thumb_func
yp3_recovery_watchdog_refresh:
    ldr r0,=WDT_BASE + WDT_REFRESH
    ldr r1,=WDT_REFRESH_KEY
    str r1,[r0]
    dsb
    bx lr
    .ltorg
    .size yp3_recovery_watchdog_refresh,.-yp3_recovery_watchdog_refresh

    .section .text.yp3_recovery_stop,"ax",%progbits
    .global yp3_recovery_watchdog_stop
    .type yp3_recovery_watchdog_stop,%function
    .thumb_func
yp3_recovery_watchdog_stop:
    ldr r0,=WDT_BASE
    movs r1,#0
    str r1,[r0,#WDT_SELECTOR]   /* 0 = disabled */
    dsb
    bx lr
    .ltorg
    .size yp3_recovery_watchdog_stop,.-yp3_recovery_watchdog_stop
