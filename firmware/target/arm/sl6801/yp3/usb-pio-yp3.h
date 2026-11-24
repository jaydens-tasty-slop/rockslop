/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef USB_PIO_YP3_H
#define USB_PIO_YP3_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "cpu.h"

/* Offsets are from the USB controller base USB_BASE (sl6801.h). */
static inline uint8_t yp3_usb_reg_read8(uint16_t offset)
{ return *(volatile uint8_t *)(USB_BASE + offset); }
static inline uint16_t yp3_usb_reg_read16(uint16_t offset)
{ return *(volatile uint16_t *)(USB_BASE + offset); }
static inline uint32_t yp3_usb_reg_read32(uint16_t offset)
{ return *(volatile uint32_t *)(USB_BASE + offset); }
static inline void yp3_usb_reg_write8(uint16_t offset, uint8_t value)
{ *(volatile uint8_t *)(USB_BASE + offset) = value; }
static inline void yp3_usb_reg_write16(uint16_t offset, uint16_t value)
{ *(volatile uint16_t *)(USB_BASE + offset) = value; }
static inline void yp3_usb_reg_write32(uint16_t offset, uint32_t value)
{ *(volatile uint32_t *)(USB_BASE + offset) = value; }

enum yp3_usb_pio_result {
    YP3_USB_PIO_IDLE = 0,
    YP3_USB_PIO_ACTIVE,
    YP3_USB_PIO_DONE,
    YP3_USB_PIO_SHORT,
    YP3_USB_PIO_ZLP,
    YP3_USB_PIO_BUSY,
    YP3_USB_PIO_INVALID,
    YP3_USB_PIO_OVERFLOW,
    YP3_USB_PIO_BAD_COUNT,
    YP3_USB_PIO_STALL,
    YP3_USB_PIO_OVERRUN,
    YP3_USB_PIO_CONFIG
};

/* Caller owns INDEX and packet bounds. Full words go through the 32-bit
 * FIFO port. Write tail bytes use successive byte lanes +0/+1/+2, as ROM
 * 0x5338/0x5378 do; read tail bytes all come from +0 (see the .c file). */
void yp3_usb_pio_fifo_write(unsigned endpoint, const uint8_t *src,
                            uint32_t length);
void yp3_usb_pio_fifo_read(unsigned endpoint, uint8_t *dst, uint32_t copy,
                           uint32_t drain);

/* Caller selects EP1 and serializes INDEX/IRQ ownership around every call.
 * State is caller-owned and zero-initialized before first use. A successful
 * IN start queues one packet; completion is only reported by a later IRQ
 * service after TX ready and FIFO-not-empty both clear. */
struct yp3_usb_pio_in {
    const uint8_t *buffer;
    uint32_t length;
    uint32_t queued;
    uint16_t last_csr;
    uint16_t observed_flags; /* raw family CSR flags, not all fatal errors */
    uint8_t active;
    uint8_t in_flight;
    uint8_t append_zlp;
    uint8_t zlp_queued;
    enum yp3_usb_pio_result result;
    uint8_t endpoint; /* selected IN FIFO: 1 or 2 */
};

/* OUT service consumes at most one 64-byte packet per call, copies no more
 * than capacity, and drains excess bytes before returning OVERFLOW. */
struct yp3_usb_pio_out {
    uint8_t *buffer;
    uint32_t capacity;
    uint32_t received;
    uint8_t active;
    enum yp3_usb_pio_result result;
};

enum yp3_usb_pio_result yp3_usb_pio_in_start_ep(struct yp3_usb_pio_in *state,
                                                 unsigned endpoint,
                                                 const void *buffer,
                                                 uint32_t length,
                                                 bool append_zlp);
enum yp3_usb_pio_result yp3_usb_pio_in_irq(struct yp3_usb_pio_in *state);
enum yp3_usb_pio_result yp3_usb_pio_out_start(struct yp3_usb_pio_out *state,
                                               void *buffer,
                                               uint32_t capacity);
enum yp3_usb_pio_result yp3_usb_pio_out_irq(struct yp3_usb_pio_out *state);

#endif
