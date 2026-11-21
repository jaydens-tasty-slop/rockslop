/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Fault entry: save the callee-saved registers, move to a dedicated stack
 * (the faulting one may be the problem), record the fault in retained SRAM
 * and park without feeding the recovery watchdog. Its reset returns a dev
 * build to ROM USB; a release build (YP3_WATCHDOG_POWEROFF) boots and
 * powers off. */
    .syntax unified
    .cpu cortex-m4
    .thumb
#include "recovery-yp3.h"
#include "fault-yp3.h"

#define EXC_RETURN_PSP       4  /* EXC_RETURN bit 2: frame is on the PSP */

    .section .text.yp3_fault,"ax",%progbits

    .global hardfault_handler
    .global memmanage_handler
    .global busfault_handler
    .global usagefault_handler
    .global yp3_fault_unhandled_entry
    .global yp3_fault_divzero_entry
    .thumb_func
hardfault_handler:
    .thumb_func
memmanage_handler:
    .thumb_func
busfault_handler:
    .thumb_func
usagefault_handler:
    movs    r2, #FAULT_KIND_EXCEPTION
    b       1f
    .thumb_func
yp3_fault_unhandled_entry:
    movs    r2, #FAULT_KIND_UNHANDLED
    b       1f
    .thumb_func
yp3_fault_divzero_entry:
    movs    r2, #FAULT_KIND_DIVZERO
1:  cpsid   i
    ldr     r12, =yp3_fault_saved_r4_r11
    stmia   r12, {r4-r11}
    mov     r1, lr                          /* EXC_RETURN (or caller LR) */
    tst     lr, #EXC_RETURN_PSP
    ite     eq
    mrseq   r0, msp
    mrsne   r0, psp                         /* exception frame */
    ldr     r12, =__yp3_fault_emergency_stack_end
    mov     sp, r12
    bl      yp3_fault_record
    bl      yp3_usb_drop_softconn
    yp3_recovery_wait
    .ltorg
