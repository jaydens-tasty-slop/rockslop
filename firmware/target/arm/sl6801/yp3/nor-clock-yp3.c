/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include <stdint.h>
#include "recovery-yp3.h"
#include "mmio-sl6801.h"


/* CLK_SEL_2B (clock id 0x2b): bits 11:8 divisor-1, bits 5:4 source
 * (3 = PLL2, 384 MHz), bit 0 gate. Stock firmware runs 384/8 = 48 MHz with quad XIP
 * (crt0 selects quad read 0xEB); we use 384/7 = 54.9 MHz.
 *
 * XIP time is pure SPI clocks (~28 per 4-byte fetch), so it scales with this
 * clock. The P25Q16H is rated 104 MHz for 0xEB, but its data comes up to
 * 7 ns (tCLQV) after the falling edge and the controller samples about half
 * a clock later: dev `norscan stress` (2026-10-18) read every block wrong at
 * 76.8 MHz (6.5 ns half period), none of 64 MB at 64 MHz (7.8 ns) or
 * 54.9 MHz (9.1 ns). No controller or clock-selector bit moved the sample
 * point. 64 MHz would leave under ~1 ns for heat and supply, so 54.9 MHz. */
/* The fields this code owns (CLK_SEL_2B_*, sl6801.h); other bits are kept
 * as found. */
#define NOR_CLK_FIELDS      (CLK_SEL_2B_DIV_MASK | CLK_SEL_2B_SOURCE_MASK | \
                             CLK_SEL_2B_GATE)
/* Bootloader value 0x801: divisor field 8 (/9), source 0, gate on. What
 * source 0 is has not been established. */
#define NOR_CLK_COLD        (CLK_SEL_2B_DIV(9) | CLK_SEL_2B_GATE)
#define NOR_CLOCK           (CLK_SEL_2B_DIV(7) | CLK_SEL_2B_SOURCE_PLL2 | \
                             CLK_SEL_2B_GATE)

#define CLOCK_CODE \
    __attribute__((section(".yp3_hal_provider_extra"), noinline))
extern uint32_t yp3_nor_clock32_set(uint32_t expected);

CLOCK_CODE static void clock_refused(void) __attribute__((noreturn));
CLOCK_CODE static void clock_refused(void)
{
    __asm__ volatile("cpsid i\n\tdsb" ::: "memory");
    yp3_recovery_park();
}

/* Called from crt0 once .data (holding the SRAM clock leaf) is copied, with
 * IRQs masked and nothing else running. */
CLOCK_CODE void yp3_nor_clock_init(void)
{
    uint32_t mask;
    uint32_t clock = REG(CLK_SEL_2B);
    __asm__ volatile("mrs %0,primask" : "=r"(mask));
    /* Expected state: IRQs masked, bootloader clock, controller enabled and
     * XIP mapped by crt0. Anything else: park rather than guess. */
    if (mask != 1u || (clock & NOR_CLK_FIELDS) != NOR_CLK_COLD ||
        REG(SPI_NOR_BASE + SPI_NOR_CONTROL) != 1u ||
        REG(SPI_NOR_BASE + SPI_NOR_XIP_MAP) != SPI_NOR_XIP_MAP_ON)
        clock_refused();
    /* Only the NOR clock changes: the XIP read mode and mapping, flash
     * status (QE) and pads set up by crt0 stay as they are. */
    if ((yp3_nor_clock32_set((clock & ~NOR_CLK_FIELDS) | NOR_CLOCK) &
         NOR_CLK_FIELDS) != NOR_CLOCK)
        clock_refused();
}
