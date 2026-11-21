/* SPDX-License-Identifier: GPL-2.0-or-later */
/* NOR/XIP clock sequence (clock id 0x2b): gate off,
 * divisor, gate on, select PLL2. r0 = expected final CLK_SEL_2B value; its
 * bits 11:8 give the divisor. Runs from SRAM: no XIP fetch while gated.
 * If the readback differs, park (never return to XIP on an unknown clock). */
#include "config.h"
#include "cpu.h"

/* CLK_SEL_2B and its fields: sl6801.h */
.syntax unified
.cpu cortex-m4
.thumb
.section .yp3_nor_clock_ramcode,"ax",%progbits
.balign 4
.global yp3_nor_clock32_set
.type yp3_nor_clock32_set,%function
.thumb_func
yp3_nor_clock32_set:
    mov r1, r0                  /* Caller-derived full expected readback. */
    mrs r3, primask
    cpsid i
    dsb
    ldr r2, =CLK_SEL_2B
    ldr r0, [r2]
    bic r0, r0, #CLK_SEL_2B_GATE
    str r0, [r2]                 /* Gate off (selector 0x2b = 43). */
    ldr r0, [r2]
    bic r0, r0, #CLK_SEL_2B_DIV_MASK
    and r12, r1, #CLK_SEL_2B_DIV_MASK
    orr r0, r0, r12
    str r0, [r2]                 /* Divisor from r1, source unchanged. */
    ldr r0, [r2]
    orr r0, r0, #CLK_SEL_2B_GATE
    str r0, [r2]                 /* Gate on before source selection. */
    ldr r0, [r2]
    orr r0, r0, #CLK_SEL_2B_SOURCE_PLL2
    str r0, [r2]                 /* Source 3 = PLL2. */
    dsb
    isb
    ldr r0, [r2]
    cmp r0, r1
    bne yp3_nor_clock32_park
    msr primask, r3
    bx lr
.global yp3_nor_clock32_park
yp3_nor_clock32_park:
    ldr r2, =SL6801_COUNTER_LO
1:  ldr r0, [r2]                /* No return to XIP after uncertain clock. */
    b 1b
.ltorg
.size yp3_nor_clock32_set, .-yp3_nor_clock32_set
