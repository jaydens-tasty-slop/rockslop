/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SL6801/YP3 startup. */
    .syntax unified
    .cpu cortex-m4
    .thumb
#include "xip-setup.inc"
#include "recovery-yp3.h"

/* Cortex-M4 system control block (ARMv7-M architecture); SCB_ICSR,
 * SCB_VTOR, SCB_CPACR, FPU_FPCCR and NVIC_WORDS are in sl6801.h. */
#define SCB_ICSR_PENDSVCLR      (1 << 27)
#define SCB_ICSR_PENDSTCLR      (1 << 25)
#define CPACR_CP10_CP11         0x00f00000  /* bits 23:20: FPU access */
#define FPCCR_ASPEN_LSPEN       0xc0000000  /* bits 31:30: auto/lazy FP
                                               stacking */
#define CONTROL_SPSEL           2           /* thread mode uses the PSP */

/* Rockbox's stack fill pattern (checked by the thread stack usage code). */
#define STACK_FILL              0xdeadbeef

/* RAM vector table: 16 Cortex-M exception vectors + 128 SoC IRQs, 4 bytes
 * each. It must match the .vectors.platform table below. */
#define VECTOR_IRQS             128
#define VECTOR_TABLE_BYTES      ((16 + VECTOR_IRQS) * 4)

    /*
     * The stock loader copies the first 4 KiB of the FIRM to 0x804c00 and
     * enters that SRAM copy.  This stub uses only registers and a literal in
     * the copied bytes, then transfers to the absolute XIP continuation.
     * Keep this section first in .text and below the copy boundary.
     */
    .section .init.yp3_stock_stub,"ax",%progbits
    .global yp3_stock_stub_entry
    .global yp3_link_only_entry
    .type yp3_stock_stub_entry, %function
    .type yp3_link_only_entry, %function
    .thumb_func
yp3_link_only_entry:
yp3_stock_stub_entry:
#ifdef YP3_RECOVERY_WATCHDOG
    /* Relative branch within the copied block; the loader's LR and masks
     * are preserved. The early entry arms PMU reason 7 and the recovery
     * watchdog, then comes back to yp3_stock_stub_xip_setup. */
    b.w     yp3_early_recovery_entry
    .global yp3_stock_stub_xip_setup
    .type yp3_stock_stub_xip_setup,%function
    .thumb_func
yp3_stock_stub_xip_setup:
#endif
    cpsid   i
    /* Stock loader setup alone does not establish the high-flash aperture.
     * This must run in the copied SRAM bytes, before the first XIP fetch. */
    yp3_prepare_xip yp3_stock_stub_refused
    ldr     r0, 1f
    bx      r0
    .global yp3_stock_stub_refused
yp3_stock_stub_refused:
#ifdef YP3_RECOVERY_WATCHDOG
    /* Never feed the watchdog: it expires and the player returns to ROM
     * USB. The wait keeps reading a bus register (see recovery-yp3.h). */
    yp3_recovery_wait
#else
    /* Keep refusal in SRAM; the caller must use physical ROM recovery. */
    b       yp3_stock_stub_refused
#endif
    .balign 4
1:  .word   yp3_native_xip_entry + 1
    .ltorg
    .size yp3_stock_stub_entry, .-yp3_stock_stub_entry
#ifdef YP3_RECOVERY_WATCHDOG
    .size yp3_stock_stub_xip_setup, .-yp3_stock_stub_xip_setup
#endif

    /* XIP entry: IRQs masked, still on the stock loader's stack. */
    .section .init.yp3_xip_entry,"ax",%progbits
    .global reset_handler
    .type reset_handler, %function
    .thumb_func
reset_handler:
    .global yp3_native_xip_entry
    .type yp3_native_xip_entry, %function
    .thumb_func
yp3_native_xip_entry:
    cpsid   i
    /* Only CPU0 runs Rockbox. */
    ldr     r1, =SYS_CPU_SELECT
    ldr     r1, [r1]
    tst     r1, #SYS_CPU_SELECT_SECONDARY
    bne     startup_hold

    /* Disable and clear every NVIC interrupt the stock loader left behind
     * (it leaves IRQ18 enabled), plus pending SysTick/PendSV. */
    ldr     r0, =NVIC_ICER_BASE /* ICER0..7 */
    ldr     r1, =NVIC_ICPR_BASE /* ICPR0..7 */
    mov     r2, #-1
    movs    r3, #NVIC_WORDS
1:  str     r2, [r0], #4
    str     r2, [r1], #4
    subs    r3, #1
    bne     1b
    ldr     r0, =SCB_ICSR
    ldr     r1, =(SCB_ICSR_PENDSVCLR | SCB_ICSR_PENDSTCLR)
    str     r1, [r0]

    /* All code is soft-float: disable FP access and lazy FP stacking. */
    ldr     r1, =SCB_CPACR
    ldr     r0, [r1]
    bic     r0, r0, #CPACR_CP10_CP11
    str     r0, [r1]
    ldr     r1, =FPU_FPCCR
    ldr     r0, [r1]
    bic     r0, r0, #FPCCR_ASPEN_LSPEN
    str     r0, [r1]
    dsb
    isb

    /* Fill both stacks with the marker Rockbox uses for usage checks, then
     * move to our own MSP. */
    ldr     r0, =irqstackbegin
    ldr     r1, =stackend
    ldr     r2, =STACK_FILL
1:  cmp     r0, r1
    itt     lo
    strlo   r2, [r0], #4
    blo     1b
    ldr     r0, =irqstackend
    msr     msp, r0
    movs    r1, #0
    msr     control, r1
    isb

    /* Copy vectors and .data from flash, zero .bss and the framebuffer. */
    ldr     r0, =__vectors_arm
    ldr     r1, =__vectors_load
    ldr     r2, =__vectors_arm + VECTOR_TABLE_BYTES
    bl      copy_words
    ldr     r0, =_databegin
    ldr     r1, =_datacopy
    ldr     r2, =_dataend
    bl      copy_words
    ldr     r0, =_bssbegin
    ldr     r1, =_bssend
    bl      zero_words
    ldr     r0, =__framebuffer_begin
    ldr     r1, =__framebuffer_end
    bl      zero_words

    /* Full-speed NOR/XIP clock; its SRAM leaf is in the .data just copied. */
    bl      yp3_nor_clock_init

    ldr     r0, =SCB_VTOR
    ldr     r1, =__vectors_arm
    str     r1, [r0]
    dsb
    isb

    /* Threads run on the PSP. */
    ldr     r0, =stackend
    msr     psp, r0
    movs    r0, #CONTROL_SPSEL
    msr     control, r0
    isb
    b       main

/* r0 = dst, r1 = src, r2 = dst end (word aligned) */
copy_words:
    cmp     r0, r2
    ittt    lo
    ldrlo   r3, [r1], #4
    strlo   r3, [r0], #4
    blo     copy_words
    bx      lr

/* r0 = start, r1 = end (word aligned) */
zero_words:
    movs    r2, #0
1:  cmp     r0, r1
    itt     lo
    strlo   r2, [r0], #4
    blo     1b
    bx      lr

startup_hold:
    cpsid   i
#ifdef YP3_RECOVERY_WATCHDOG
    yp3_recovery_wait
#else
    wfi
    b       startup_hold
#endif
    .size yp3_native_xip_entry, .-yp3_native_xip_entry
    .ltorg

    /* 16 Cortex-M vectors (system-arm-micro.c) are followed by 128 IRQs. */
    .section .vectors.platform,"a",%progbits
    .global __vectors_platform
__vectors_platform:
    .rept VECTOR_IRQS
    .word UIE + 1
    .endr
