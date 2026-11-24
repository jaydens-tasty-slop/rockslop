/* SPDX-License-Identifier: GPL-2.0-or-later */
/* XIP setup for the Rockbox entry (crt0.S), assembly only.
 *
 * The stock loader copies the first 4 KiB of the FIRM to SRAM and calls it;
 * everything after that runs from the flash window (XIP) at 0xc00000.
 * Before the first XIP fetch the flash controller must be switched to the
 * state needed for XIP: register +0x24 = 0x40 (required before Rockbox's
 * first XIP fetch) and, for speed, quad I/O reads.
 * The macro runs from the SRAM copy; it uses no stack, calls nothing and
 * leaves clocks and interrupts alone. Clobbers r0-r2 and the flags.
 *
 * It refuses (branches to the caller's label, which must also be in SRAM)
 * unless it was entered as the stock loader enters the FIRM - thread mode,
 * privileged, PRIMASK set, BASEPRI and FAULTMASK clear, on CPU0 - and the
 * controller is enabled and still in the read mode the bootloader leaves.
 * That is the only state this sequence has been tested from. Nothing here
 * stops the second core or DMA; changing the read mode assumes nothing else
 * is fetching from flash at the time. */
#include "config.h"
#include "cpu.h"

/* SPI NOR flash controller registers: sl6801.h. XIP read mode: the command
 * word the controller uses for XIP fetches.
 * The low byte is the flash opcode. The rest matches the controller's raw
 * command word for that opcode with bit 22 cleared and the
 * common bits 0x00a08000 added; the ROM initialiser also sets bit 25.
 * The meaning of the individual bits is not known. */
/* Cold value, left by the boot ROM and bootloader: single-wire READ 0x03. */
#define XIP_READ_SINGLE         0x02a09503
/* Quad I/O fast read 0xEB. Needs the flash's QE bit, which this part has
 * set (SR2 == 2); we rely on it being set. */
#define XIP_READ_QUAD           0x02a6bdeb

/* NOR pads: bank 0 pads 0-5 (flash IO0-3, CLK, CS) get mux function 2, set
 * before switching to quad reads. They share the mux
 * word GPIO_MUX(0, 0..7) (yp3/gpio-yp3.h, 4 bits per pad) with pads 6-7,
 * whose fields are kept. */
#define NOR_PAD_MUX             GPIO_BASE
#define NOR_PAD_MUX_KEEP        0xff000000  /* pads 6-7 */
#define NOR_PAD_MUX_FUNC2       0x00222222  /* pads 0-5: function 2 */

    .macro yp3_prepare_xip refusal
    /* Entry state: thread mode, privileged, only PRIMASK masking. */
    mrs     r0, ipsr
    cmp     r0, #0
    bne     \refusal
    mrs     r0, control
    tst     r0, #1                  /* CONTROL.nPRIV */
    bne     \refusal
    mrs     r0, primask
    cmp     r0, #1
    bne     \refusal
    mrs     r0, basepri
    cmp     r0, #0
    bne     \refusal
    mrs     r0, faultmask
    cmp     r0, #0
    bne     \refusal
    /* CPU0 only. */
    ldr     r0, =SYS_CPU_SELECT
    ldr     r0, [r0]
    tst     r0, #SYS_CPU_SELECT_SECONDARY
    bne     \refusal
    /* Controller as the bootloader leaves it. */
    ldr     r0, =SPI_NOR_BASE
    ldr     r1, [r0, #SPI_NOR_CONTROL]
    cmp     r1, #1
    bne     \refusal
    ldr     r1, [r0, #SPI_NOR_XIP_READ]
    ldr     r2, =XIP_READ_SINGLE
    cmp     r1, r2
    bne     \refusal
    ldr     r1, [r0, #SPI_NOR_XIP_MAP]
    cmp     r1, #0
    beq     .Lyp3_xip_config\@
    cmp     r1, #SPI_NOR_XIP_MAP_BOOTLOADER
    bne     \refusal
.Lyp3_xip_config\@:
    movs    r1, #SPI_NOR_XIP_MAP_ON
    str     r1, [r0, #SPI_NOR_XIP_MAP]
    dsb
    isb
    ldr     r1, [r0, #SPI_NOR_XIP_MAP]
    cmp     r1, #SPI_NOR_XIP_MAP_ON
    bne     \refusal
    /* Quad reads: NOR pads to function 2, then the quad read mode. */
    ldr     r0, =NOR_PAD_MUX
    ldr     r1, [r0]
    and     r1, r1, #NOR_PAD_MUX_KEEP
    ldr     r2, =NOR_PAD_MUX_FUNC2
    orr     r1, r1, r2
    str     r1, [r0]
    ldr     r0, =SPI_NOR_BASE
    ldr     r1, =XIP_READ_QUAD
    str     r1, [r0, #SPI_NOR_XIP_READ]
    dsb
    isb
    ldr     r2, [r0, #SPI_NOR_XIP_READ]
    cmp     r1, r2
    bne     \refusal
    .endm
