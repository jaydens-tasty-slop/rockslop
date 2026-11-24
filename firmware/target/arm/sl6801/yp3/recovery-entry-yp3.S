/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Earliest firmware entry (YP3_RECOVERY_WATCHDOG builds), run from the
 * 4 KiB FIRM prefix that the stock loader copied to SRAM, before any XIP
 * fetch. It checks that it was entered exactly as the stock loader enters a
 * FIRM, sets the watchdog-reset boot reason (recovery-flags-yp3.c: 7 = ROM
 * USB in dev builds) and arms the ~16 s watchdog (recovery-hw-yp3.S), then
 * continues to the XIP setup in crt0.S. All code
 * here must stay inside the copied prefix, below the temporary stack (app.lds
 * asserts this); calls and branches are PC-relative. */
#include "config.h"
#include "cpu.h"
#include "pmu-yp3.h"
#include "recovery-yp3.h"
.syntax unified
.cpu cortex-m4
.thumb
/* Temporary stack: its top is 16 bytes below the end of the 4 KiB copied
 * prefix; app.lds keeps code below loadaddress + 0xbf0, leaving it ~1 KiB. */
#define YP3_EARLY_STACK (SL6801_CORE_RAM_BASE + 0x1000 - 0x10)
/* Stock loader entry context:
 * LoadFirmwareFromFlash calls the copied prefix with this MSP and LR. */
#define YP3_EARLY_LOADER_MSP 0x83be58
#define YP3_EARLY_LOADER_LR 0x8204d3
/* SCB CPUID of this core: ARM Cortex-M4 r0p1. */
#define YP3_EARLY_CPUID 0x410fc241
/* VTOR as left by the ROM (SRAM base, ROM vector template). */
#define YP3_EARLY_VTOR 0x800000

/* Cortex-M4 system control registers (ARMv7-M architecture); SCB_CPUID,
 * SCB_VTOR and FPU_FPCCR are in sl6801.h. */
#define FPCCR_LSPACT    1       /* lazy FP state pending: a FP context is
                                   live */

/* Watchdog timeout calibration word: with mode 0x2001 and selector 0x71
 * it resets ~16 s after the last feed. */
#define WDT_CAL_16S     0x7fff00

/* Byte offsets of the out[] words (recovery-yp3.h) on our stack. */
#define OUT_EMERGENCY_CMD (4 * EARLY_OUT_EMERGENCY_CMD)
#define OUT_PMU_D8        (4 * EARLY_OUT_PMU_D8)
#define OUT_RESET_BYTE    (4 * EARLY_OUT_RESET_BYTE)
#define OUT_WRITE_TRIED   (4 * EARLY_OUT_WRITE_TRIED)
#define OUT_SIZE          (4 * EARLY_OUT_WORDS)
.section .init.yp3_early_recovery_entry,"ax",%progbits
.global yp3_early_recovery_entry
.type yp3_early_recovery_entry,%function
.thumb_func
yp3_early_recovery_entry:
 /* Refuse (return 0 to the loader, see early_invalid) unless the context is
  * the stock loader's: its return address, thread mode, MSP in use, IRQs
  * masked by PRIMASK only, its stack, our core type, ROM VTOR, no live FP
  * state and CPU0. APSR is saved in r12 for the refusal return. */
 mrs r12,apsr
 ldr r0,=YP3_EARLY_LOADER_LR
 cmp lr,r0
 bne early_invalid
 mrs r0,ipsr
 cmp r0,#0
 bne early_invalid
 mrs r0,control
 cmp r0,#0
 bne early_invalid
 mrs r0,primask
 cmp r0,#1
 bne early_invalid
 mrs r0,basepri
 cmp r0,#0
 bne early_invalid
 mrs r0,faultmask
 cmp r0,#0
 bne early_invalid
 mrs r0,msp
 ldr r1,=YP3_EARLY_LOADER_MSP
 cmp r0,r1
 bne early_invalid
 ldr r0,=SCB_CPUID
 ldr r0,[r0]
 ldr r1,=YP3_EARLY_CPUID
 cmp r0,r1
 bne early_invalid
 ldr r0,=SCB_VTOR
 ldr r0,[r0]
 ldr r1,=YP3_EARLY_VTOR
 cmp r0,r1
 bne early_invalid
 ldr r0,=FPU_FPCCR
 ldr r0,[r0]
 tst r0,#FPCCR_LSPACT
 bne early_invalid
 ldr r0,=SYS_CPU_SELECT
 ldr r0,[r0]
 tst r0,#SYS_CPU_SELECT_SECONDARY
 bne early_invalid
 /* Switch to the temporary stack, saving the loader's sp and every register
  * the loader may rely on; reserve the out[] words. */
 mov r0,sp
 ldr r1,=YP3_EARLY_STACK
 mov sp,r1
 push {r0,r1,r4-r11,r12,lr}
 sub sp,#OUT_SIZE
 mov r0,sp
 bl yp3_early_recovery_arm_flags
 cbz r0,early_arm_refused
 ldr r0,=WDT_CAL_16S
 ldr r1,[sp,#OUT_EMERGENCY_CMD]
 ldr r2,[sp,#OUT_PMU_D8]
 bl yp3_recovery_watchdog_prepare
 /* Armed. Restore the loader's registers and stack, then do the XIP setup
  * (crt0.S) without checking anything further. */
 add sp,#OUT_SIZE
 pop {r0,r1,r4-r11,r12,lr}
 mov sp,r0
 b.w yp3_stock_stub_xip_setup
early_arm_refused:
 /* Arming failed. If no PMU write can have landed, return 0 to the loader
  * like the context checks do; otherwise the reason flags may be half set,
  * so reset instead. */
 ldr r0,[sp,#OUT_WRITE_TRIED]
 cbnz r0,early_partial_restart
 add sp,#OUT_SIZE
 pop {r0,r1,r4-r11,r12,lr}
 mov sp,r0
 movs r0,#0
 msr apsr_nzcvq,r12
 /* Return 0 with IRQs enabled, as the ordinary (non-recovery) refusal
  * did; why it enables IRQs is not documented. */
 cpsie i
 bx lr
early_partial_restart:
 /* PMU software reset after a possibly partial flag write: write
  * PMU[PMU_REG_RESET] | PMU20_RESET_REQUEST (pmu-yp3.h) through the
  * bridge without any further checks, then park reading the bus until the
  * reset hits.
  * Partial arming cannot yield reason 6 (see recovery-flags-yp3.c). */
 ldr r0,[sp,#OUT_RESET_BYTE]
 ldr r1,=PMU_BRIDGE_WDATA
 str r0,[r1]
 ldr r1,=PMU_BRIDGE_CMD
 /* write PMU_REG_RESET: load the command, then set START */
 ldr r0,=(PMU_BRIDGE_CMD_WRITE | \
          (PMU_REG_RESET << PMU_BRIDGE_CMD_INDEX_SHIFT))
 str r0,[r1]
 ldr r0,=(PMU_BRIDGE_START | PMU_BRIDGE_CMD_WRITE | \
          (PMU_REG_RESET << PMU_BRIDGE_CMD_INDEX_SHIFT))
 str r0,[r1]
 dsb
 ldr r0,=SL6801_COUNTER_LO
early_restart_wait:
 ldr r1,[r0]
 b early_restart_wait
early_invalid:
 /* Not entered by the stock loader: return 0 with the caller's flags. */
 movs r0,#0
 msr apsr_nzcvq,r12
 bx lr
.ltorg
.size yp3_early_recovery_entry,.-yp3_early_recovery_entry

/* 32-bit register accessors for recovery-flags-yp3.c, kept in the copied
 * prefix (the C code must not call anything in XIP). */
.global yp3_early_recovery_read32,yp3_early_recovery_write32
.type yp3_early_recovery_read32,%function
.thumb_func
yp3_early_recovery_read32:
 ldr r0,[r0]
 bx lr
.type yp3_early_recovery_write32,%function
.thumb_func
yp3_early_recovery_write32:
 str r1,[r0]
 bx lr
