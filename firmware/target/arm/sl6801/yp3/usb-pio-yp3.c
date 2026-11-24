/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Bounded EP1 PIO packet service. No startup, IRQ dispatch, or kernel calls.
 * The per-packet paths run from SRAM (ICODE_ATTR): from uncached XIP they
 * outlasted a full-speed packet time and every packet waited a frame. */
#include "config.h"
#include "usb-pio-yp3.h"
#include "usb-regs-yp3.h"

/* Registers (indexed: they act on the endpoint the caller selected with
 * INDEX) and bits: usb-regs-yp3.h. */

void ICODE_ATTR yp3_usb_pio_fifo_write(unsigned endpoint, const uint8_t *src,
                                       uint32_t length)
{
    uint16_t fifo = R_FIFO(endpoint);
    while (length >= 4) {
        uint32_t word = (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
                        ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
        yp3_usb_reg_write32(fifo, word);
        src += 4;
        length -= 4;
    }
    for (unsigned lane = 0; lane < length; ++lane)
        yp3_usb_reg_write8(fifo + (uint16_t)lane, *src++);
}

void ICODE_ATTR yp3_usb_pio_fifo_read(unsigned endpoint, uint8_t *dst,
                                      uint32_t copy, uint32_t drain)
{
    uint16_t fifo = R_FIFO(endpoint);
    uint32_t pos = 0;
    if (!((uintptr_t)dst & 3u)) {
        /* Usual case (aligned transfer buffer): whole words straight in. */
        uint32_t *d = (uint32_t *)dst;
        while (drain >= 4 && pos + 4 <= copy) {
            *d++ = yp3_usb_reg_read32(fifo);
            pos += 4;
            drain -= 4;
        }
    }
    while (drain >= 4) {
        uint32_t word = yp3_usb_reg_read32(fifo);
        for (unsigned lane = 0; lane < 4; ++lane, ++pos)
            if (pos < copy) dst[pos] = (uint8_t)(word >> (lane * 8));
        drain -= 4;
    }
    /* Tail bytes all come from the FIFO base: byte reads at +1/+2 return
     * garbage (seen as truncated 4n+3 byte serial packets). */
    for (unsigned lane = 0; lane < drain; ++lane) {
        uint8_t byte = yp3_usb_reg_read8(fifo);
        if (pos < copy) dst[pos] = byte;
        ++pos;
    }
}

/* TXCSR writes carry the fixed high byte plus the commands wanted; status,
 * flush and toggle bits read back are never written again. */
static void ICODE_ATTR queue_packet(struct yp3_usb_pio_in *s)
{
    uint32_t amount = s->length - s->queued;
    if (amount > YP3_USB_MPS) amount = YP3_USB_MPS;
    if (amount) {
        yp3_usb_pio_fifo_write(s->endpoint, s->buffer + s->queued, amount);
        s->queued += amount;
    } else {
        s->zlp_queued = 1;
    }
    s->in_flight = 1;
    /* Fixed peripheral PIO mode plus TXPKTRDY. Do not replay sampled modes. */
    yp3_usb_reg_write16(R_TXCSR, INCSR_PIO_MODE | INCSR_INPKTRDY);
}

enum yp3_usb_pio_result yp3_usb_pio_in_start_ep(struct yp3_usb_pio_in *s,
                                                 unsigned endpoint,
                                                 const void *buffer,
                                                 uint32_t length,
                                                 bool append_zlp)
{
    if (!s || endpoint < 1 || endpoint > 2 || (length && !buffer))
        return YP3_USB_PIO_INVALID;
    if (s->active) return YP3_USB_PIO_BUSY;
    uint16_t csr = yp3_usb_reg_read16(R_TXCSR);
    if (csr & INCSR_SENTSTALL) {
        s->last_csr = csr;
        s->observed_flags = csr & (INCSR_OBSERVED_BIT2 | INCSR_SENTSTALL);
        return s->result = YP3_USB_PIO_STALL;
    }
    if (csr & (INCSR_INPKTRDY | INCSR_FIFONOTEMPTY)) return YP3_USB_PIO_BUSY;
    if ((csr & INCSRH_MASK) != INCSR_PIO_MODE)
        return s->result = YP3_USB_PIO_CONFIG;

    s->buffer = (const uint8_t *)buffer;
    s->length = length;
    s->queued = 0;
    s->append_zlp = (uint8_t)append_zlp;
    s->zlp_queued = 0;
    s->endpoint = (uint8_t)endpoint;
    s->last_csr = csr;
    s->observed_flags = csr & (INCSR_OBSERVED_BIT2 | INCSR_SENTSTALL);
    s->active = 1;
    s->result = YP3_USB_PIO_ACTIVE;
    queue_packet(s); /* A zero-length transfer queues one ZLP. */
    return s->result;
}

enum yp3_usb_pio_result ICODE_ATTR yp3_usb_pio_in_irq(struct yp3_usb_pio_in *s)
{
    if (!s) return YP3_USB_PIO_INVALID;
    if (!s->active) return s->result;
    uint16_t csr = yp3_usb_reg_read16(R_TXCSR);
    s->last_csr = csr;
    s->observed_flags = csr & (INCSR_OBSERVED_BIT2 | INCSR_SENTSTALL);
    if (csr & INCSR_SENTSTALL) {
        s->active = s->in_flight = 0;
        return s->result = YP3_USB_PIO_STALL;
    }
    if ((csr & INCSRH_MASK) != INCSR_PIO_MODE) {
        s->active = s->in_flight = 0;
        return s->result = YP3_USB_PIO_CONFIG;
    }
    if ((csr & (INCSR_INPKTRDY | INCSR_FIFONOTEMPTY)) != 0)
        return YP3_USB_PIO_ACTIVE;
    s->in_flight = 0;
    if (s->queued < s->length) {
        queue_packet(s);
        return YP3_USB_PIO_ACTIVE;
    }
    if (!s->zlp_queued && s->append_zlp &&
        s->length && !(s->length % YP3_USB_MPS)) {
        queue_packet(s);
        return YP3_USB_PIO_ACTIVE;
    }
    s->active = 0;
    return s->result = YP3_USB_PIO_DONE;
}

enum yp3_usb_pio_result yp3_usb_pio_out_start(struct yp3_usb_pio_out *s,
                                               void *buffer,
                                               uint32_t capacity)
{
    if (!s || (capacity && !buffer)) return YP3_USB_PIO_INVALID;
    if (s->active) return YP3_USB_PIO_BUSY;
    uint16_t csr = yp3_usb_reg_read16(R_RXCSR);
    if (csr & OUTCSR_SENTSTALL) return s->result = YP3_USB_PIO_STALL;
    if (csr & OUTCSR_OVERRUN) return s->result = YP3_USB_PIO_OVERRUN;
    if (csr & OUTCSRH_MASK) return s->result = YP3_USB_PIO_CONFIG;
    s->buffer = (uint8_t *)buffer;
    s->capacity = capacity;
    s->received = 0;
    s->active = 1;
    return s->result = YP3_USB_PIO_ACTIVE;
}

enum yp3_usb_pio_result ICODE_ATTR
yp3_usb_pio_out_irq(struct yp3_usb_pio_out *s)
{
    if (!s) return YP3_USB_PIO_INVALID;
    if (!s->active) return s->result;
    uint16_t csr = yp3_usb_reg_read16(R_RXCSR);
    if (csr & OUTCSR_SENTSTALL) {
        s->active = 0;
        return s->result = YP3_USB_PIO_STALL;
    }
    if (csr & OUTCSR_OVERRUN) {
        s->active = 0;
        return s->result = YP3_USB_PIO_OVERRUN;
    }
    if (csr & OUTCSRH_MASK) {
        s->active = 0;
        return s->result = YP3_USB_PIO_CONFIG;
    }
    if (!(csr & OUTCSR_OUTPKTRDY)) return YP3_USB_PIO_ACTIVE;
    uint16_t count = yp3_usb_reg_read16(R_RXCOUNT);
    if (count > YP3_USB_MPS) {
        s->active = 0;
        return s->result = YP3_USB_PIO_BAD_COUNT;
    }
    uint32_t room = s->capacity - s->received;
    uint32_t copy = count < room ? count : room;
    uint8_t *dst = s->buffer ? s->buffer + s->received : 0;
    /* excess is drained without storing */
    yp3_usb_pio_fifo_read(1, dst, copy, count);
    s->received += copy;
    /* Writing 0 clears OUTPKTRDY, handing the FIFO back for the next packet;
     * the PIO profile has no other RXCSR bits to keep. */
    yp3_usb_reg_write16(R_RXCSR, 0);
    if (copy != count) {
        s->active = 0;
        return s->result = YP3_USB_PIO_OVERFLOW;
    }
    if (!count) {
        s->active = 0;
        return s->result = YP3_USB_PIO_ZLP;
    }
    if (s->received == s->capacity) {
        s->active = 0;
        return s->result = YP3_USB_PIO_DONE;
    }
    if (count < YP3_USB_MPS) {
        s->active = 0;
        return s->result = YP3_USB_PIO_SHORT;
    }
    return YP3_USB_PIO_ACTIVE;
}
