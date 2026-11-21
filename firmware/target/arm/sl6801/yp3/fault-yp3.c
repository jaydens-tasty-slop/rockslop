/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include "system.h"
#include "crc32.h"
#include "dev-yp3.h"
#include "fault-yp3.h"
#include "regs/cortex-m/cm_scb.h"
#include <stdint.h>
#include <stddef.h>

/* 128-byte record at the start of the watchdog-retained SRAM region
 * (app.lds .yp3_retained); decoded by yp3dev/yp3.py fault. */
struct fault_record {
    uint32_t magic, version_size, flags, exc_return;
    uint32_t msp, psp, control, ipsr;
    uint32_t cfsr, hfsr, mmfar, bfar;
    uint32_t r4_r11[8];
    uint32_t frame_sp;
    uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
    uint32_t masks, crc32, commit;
};
_Static_assert(sizeof(struct fault_record) == 128, "fault record layout");

/* Constants shared with yp3dev/yp3.py decode_fault(). */
#define FAULT_MAGIC  0x46594a59u   /* bytes "YJYF" */
#define FAULT_COMMIT 0x434f4d54u   /* bytes "TMOC"; written last, after the
                                      CRC */
#define FAULT_VERSION 2
#define FAULT_RECORD_BYTES 128u
#define FLAG_FRAME_VALID (1u << 0)  /* r0..xpsr copied from the stacked
                                       frame */
#define FLAG_UNHANDLED   (1u << 7)
#define FLAG_DIVZERO     (1u << 9)
#define MASKS_PRIMASK_SHIFT 8      /* masks word: PRIMASK in bit 8, rest 0 */

volatile struct fault_record yp3_fault_noinit_record
    __attribute__((section(".noinit.fault_record"), used, aligned(8)));
uint32_t yp3_fault_saved_r4_r11[8];

/* Exception frame stacked by the core: EXC_FRAME_WORDS words (sl6801.h). */
#define EXC_FRAME_BYTES (EXC_FRAME_WORDS * 4u)

void yp3_fault_record(uint32_t frame_sp, uint32_t exc_return, unsigned kind)
{
    volatile struct fault_record *r = &yp3_fault_noinit_record;
    uint32_t v;

    r->commit = 0;
    r->magic = FAULT_MAGIC;
    r->version_size = FAULT_VERSION | (FAULT_RECORD_BYTES << 16);
    r->flags = kind == FAULT_KIND_UNHANDLED ? FLAG_UNHANDLED :
               kind == FAULT_KIND_DIVZERO ? FLAG_DIVZERO : 0;
    r->exc_return = exc_return;
    __asm__ volatile("mrs %0, msp" : "=r"(v)); r->msp = v;
    __asm__ volatile("mrs %0, psp" : "=r"(v)); r->psp = v;
    __asm__ volatile("mrs %0, control" : "=r"(v)); r->control = v;
    __asm__ volatile("mrs %0, ipsr" : "=r"(v)); r->ipsr = v;
    r->cfsr = reg_read(CM_SCB_CFSR);
    r->hfsr = reg_read(CM_SCB_HFSR);
    r->mmfar = reg_read(CM_SCB_MMFAR);
    r->bfar = reg_read(CM_SCB_BFAR);
    for (int i = 0; i < 8; i++)
        r->r4_r11[i] = yp3_fault_saved_r4_r11[i];
    r->frame_sp = frame_sp;
    /* Only read the stacked frame if it lies in SRAM. */
    if (kind != FAULT_KIND_DIVZERO && frame_sp >= SL6801_RAM_BASE &&
        frame_sp <= SL6801_RAM_BASE + SL6801_RAM_SIZE - EXC_FRAME_BYTES &&
        !(frame_sp & 3)) {
        const uint32_t *f = (const uint32_t *)frame_sp;
        r->r0 = f[0]; r->r1 = f[1]; r->r2 = f[2]; r->r3 = f[3];
        r->r12 = f[4]; r->lr = f[5]; r->pc = f[6]; r->xpsr = f[7];
        r->flags |= FLAG_FRAME_VALID;
    }
    __asm__ volatile("mrs %0, primask" : "=r"(v));
    r->masks = v << MASKS_PRIMASK_SHIFT;
    /* Standard CRC-32 (init and final XOR 0xffffffff), as Python's
     * binascii.crc32 computes it over the first 120 bytes. */
    r->crc32 = crc_32r((const void *)r, offsetof(struct fault_record, crc32),
                       0xffffffff) ^ 0xffffffff;
    __asm__ volatile("dsb" ::: "memory");
    r->commit = FAULT_COMMIT;
    dev_log("FAULT kind=%u ipsr=%lu cfsr=%08lx pc=%08lx lr=%08lx",
                kind, (unsigned long)r->ipsr, (unsigned long)r->cfsr,
                (unsigned long)r->pc, (unsigned long)r->lr);
}
