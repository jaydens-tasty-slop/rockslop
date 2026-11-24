/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PMU_YP3_H
#define PMU_YP3_H

/* The register numbers and bits below are plain integers, so assembly
 * (recovery-entry-yp3.S) can include this header too. */
#ifndef __ASSEMBLER__
#include <stdbool.h>
#include <stdint.h>

/* The PMU is reached through an indexed register bridge at 0x40085100 that
 * only one client may use at a time. Callers take the lock around a whole
 * read/write sequence; yp3_pmu_try_lock() never blocks (IRQ-safe). */
bool yp3_pmu_try_lock(void);
void yp3_pmu_unlock(void);

/* poll_budget is the maximum number of busy-bit polls per command (each
 * poll waits 500 us; capped at 64). Return false on timeout or a bridge
 * status error. */
bool yp3_pmu_read(uint8_t index, uint32_t poll_budget, uint8_t *value);
bool yp3_pmu_write(uint8_t index, uint32_t value, uint32_t poll_budget);

/* The same commands without waiting, for interrupt context (the button
 * tick): a spin there held the audio DMA off for up to ~19 ms. Start one
 * command (lock held), then call yp3_pmu_async_done() no sooner than 500 us
 * later (a tick) until it stops returning 0: 1 = done (value set for a
 * read), -1 = failed. */
bool yp3_pmu_async_start(uint8_t index, bool write, uint8_t value);
int yp3_pmu_async_done(uint8_t *value);

/* Core voltage around a CPU clock change (power-yp3.c): call with after = false before the divider switch (raises and
 * settles; false = not raised, keep the clock) and after = true with the
 * clock now running (lowers). Thread context; does nothing until the core
 * LDO is on. */
bool yp3_core_voltage(bool max_clock, bool after);
#ifdef YP3_DEV
/* Dev shell "vcore": core voltage per clock in mV (0 = keep), applied at
 * once; logs the state. */
void yp3_core_voltage_dev(unsigned mv_normal, unsigned mv_max);
#endif

/* PMU 0x24, 0x26..0x29 as read at boot (power-yp3.c; System > Debug >
 * View HW info). */
extern uint8_t yp3_pmu_boot_regs[5];
/* Charger events (PMU_REG_EVENT30) collected by the event owner
 * (button-yp3.c) for the charge control (power-yp3.c). */
extern volatile uint8_t yp3_pmu_events30;
#endif

/* ---- Indexed 8-bit PMU registers (bridge index) ------------------------
 * Bits not named here are left as read.
 * Not from a datasheet (there is no documentation): these definitions were
 * worked out on the hardware and from the SoC's behaviour; the names are
 * ours. */

/* 0x00: mixed. Bit 0 is the low bit of the boot reason the bootloader reads
 * (reason = bit 0 | PMU[0x23] bits 7:6 << 1). Bit 3 is set in normal
 * running; role unknown. */
#define PMU_REG_00              0x00
#define PMU00_REASON_BIT0       0x01
#define PMU00_BITS21            0x06    /* cleared at boot; roles unknown */
#define PMU00_BIT3              0x08
/* Bits 7:6: power state, 2 while running (power_init), 0 to power off. */
#define PMU00_STATE_MASK        0xc0
#define PMU00_STATE_RUN         0x80

/* 0x03: rail 3 voltage code: bits other than 0x5d select
 * the voltage, 3300 mV = 0xa0. */
#define PMU_REG_03              0x03
#define PMU03_KEEP              0x5d
#define PMU03_3300MV            0xa0

/* 0x07, 0x0f: supplies the ROM's USB mode programs (values from stock
 * firmware: DCDC ~1.2 V via PMU 0x07, rail 4 ~1.2 V via PMU 0x0f). Each is
 * a read-modify-write that keeps the KEEP bits. The three 0x07 values are
 * written in this order; why it steps is unknown (usb-board-yp3.c). */
#define PMU_USB_RAIL_A          0x07
#define PMU_USB_RAIL_A_KEEP     0xc0    /* bits 7:6 preserved, role unknown */
#define PMU_USB_RAIL_A_STEP1    0x1b
#define PMU_USB_RAIL_A_STEP2    0x1d
#define PMU_USB_RAIL_A_STEP3    0x1e
#define PMU_USB_RAIL_B          0x0f
#define PMU_USB_RAIL_B_KEEP     0xf8    /* bits 7:3 preserved, role unknown */
#define PMU_USB_RAIL_B_VALUE    0x05

/* 0x11: trim from the efuse bank ; not written by us. */
#define PMU_REG_11              0x11

/* 0x17: charger control; bit 2 = charging enabled . */
#define PMU_REG_CHARGE          0x17
#define PMU17_CHARGE_ENABLE     0x04

/* 0x19, 0x1a, 0x4b: event interrupt masks, 1 = masked (only some bits read
 * back). */
#define PMU_REG_MASK19          0x19
#define PMU_REG_MASK1A          0x1a
#define PMU_REG_MASK4B          0x4b
/* The masks written at boot (power_init). */
#define PMU_MASK19_BOOT         0xe5
#define PMU_MASK1A_BOOT         0x83
#define PMU_MASK4B_BOOT         0xff

/* 0x1b: bit 7 is pulsed (set, then cleared) at the end of the boot
 * set-up; role unknown. */
#define PMU_REG_1B              0x1b
#define PMU1B_PULSE             0x80

/* 0x20: bit 7 = software reset request.
 * The PMU reset does not keep SRAM. */
#define PMU_REG_RESET           0x20
#define PMU20_RESET_REQUEST     0x80
/* The value in normal boots, written at boot (bits 3:0 and 5; roles
 * unknown); early recovery refuses to arm on anything else. */
#define PMU20_NORMAL            0x2f

/* 0x21: read for the power-off sequence; 0x22: cleared at boot. Roles
 * unknown. */
#define PMU_REG_21              0x21
#define PMU_REG_22              0x22

/* 0x23: bits 7:6 = boot reason high bits (see PMU_REG_00). 0xc0 with
 * PMU00 bit 0 = 7: ROM USB update; 0xc0 alone = 6: SD update, never
 * request it. */
#define PMU_REG_REASON          0x23
#define PMU23_REASON_MASK       0xc0

/* 0x24: power key timers. Bits 3:2 long-press reset time (1 = 8 s),
 * bits 1:0 key timers, cleared for the power-off. 0x26: read at boot
 * only. */
#define PMU_REG_24              0x24
#define PMU24_FIELDS            0x3f
#define PMU24_LONG_RESET_8S     0x04    /* bits 3:2 = 1 */
#define PMU24_KEY_TIMERS        0x03    /* bits 1:0 */
#define PMU_REG_26              0x26

/* 0x27, 0x28: power-on reasons; 0x29: power-off reason, acknowledged by
 * writing the value back. */
#define PMU_REG_PON27           0x27
#define PMU_REG_PON28           0x28
#define PMU_REG_POFF29          0x29

/* 0x2a: power input status. Bit 2 = external (USB) power present (reads
 * 0x4c with USB attached; bit 6 tracks another cable kind, unused).
 * Bit 1 = the centre key is down (0x4e while held). */
#define PMU_REG_POWER_STATUS    0x2a
#define PMU2A_KEY_DOWN          0x02
#define PMU2A_EXT_POWER         0x04

/* 0x2c: rail enables: bit 2 rail 3, bit 3 rail 4. */
#define PMU_REG_RAILS           0x2c
#define PMU2C_RAIL3             0x04
#define PMU2C_RAIL4             0x08

/* 0x2e: battery voltage conversion result, 7 bits. */
#define PMU_REG_BATTERY         0x2e
#define PMU2E_BATTERY_MASK      0x7f

/* 0x2f: cleared at boot; role unknown. */
#define PMU_REG_2F              0x2f

/* Event registers, read then written back with the value read to
 * acknowledge. One owner reads all three
 * (button-yp3.c): the key and charger events share the GPIO2.2 line. */
#define PMU_REG_EVENT30         0x30    /* charger events */
#define PMU30_CHARGE_FULL       0x02    /* bits 3/4: other charger events
                                           (unused) */
#define PMU_REG_EVENT31         0x31    /* centre (power) key events */
#define PMU31_KEY_RELEASE       0x10
#define PMU31_KEY_LONG          0x20
#define PMU31_KEY_PRESS         0x40
#define PMU_REG_EVENT4A         0x4a    /* acknowledged the same way;
                                           content unknown */

/* 0x47: bit 4 requests a battery voltage conversion (set it, wait, read
 * PMU_REG_BATTERY, clear it). Bit 0 is charger config
 * set before Rockbox runs; other bits unknown. */
#define PMU_REG_47              0x47
#define PMU47_BATTERY_MEASURE   0x10

#endif
