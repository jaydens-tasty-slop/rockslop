/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef USB_REGS_YP3_H
#define USB_REGS_YP3_H

#include <stdint.h>

/* USB controller registers, offsets from USB_BASE (sl6801.h). The core is
 * Mentor MUSB 1.900 (HWVERS read 0x0784) with MUSB's layout and
 * JZ4740-style names (IN = TX, OUT = RX).
 * TXMAXP..RXCOUNT act on the endpoint selected by INDEX. Interrupt status
 * registers clear on read, so each is read exactly once per IRQ.
 * Not from a datasheet (there is no documentation): these definitions were
 * worked out on the hardware and from the SoC's behaviour, with MUSB's
 * names where the behaviour matches. */
#define R_FADDR       0x00u
/* 0x01 POWER: USB_REG_POWER, shared (sl6801.h) */
#define R_INTRTX      0x02u
#define R_INTRRX      0x04u
#define R_INTRTXE     0x06u
#define R_INTRRXE     0x08u
#define R_INTRUSB     0x0au
#define R_INTRUSBE    0x0bu
#define R_INDEX       0x0eu
#define R_TXMAXP      0x10u
#define R_TXCSR       0x12u     /* 16 bit: low byte status/commands, high
                                   byte config */
#define R_TXCSRH      0x13u
#define R_RXMAXP      0x14u
#define R_RXCSR       0x16u     /* same split */
#define R_RXCSRH      0x17u
#define R_RXCOUNT     0x18u     /* bytes of the packet in the OUT FIFO */
/* 32-bit FIFO port per endpoint */
#define R_FIFO(ep)    (0x20u + (uint16_t)(ep) * 4u)

/* Max packet size of every endpoint this driver uses (full speed: EP0
 * control, EP1 bulk, EP2 interrupt); the FIFO sizes are not known. */
#define YP3_USB_MPS   64u

#define INTRUSB_RESET  0x04u     /* bus reset seen (peripheral mode) */
/* Per-endpoint bits of INTRTX/INTRRX and their enables. EP0 (SETUP, IN and
 * OUT alike) signals in INTRTX bit 0 only. */
#define IRQ_EP0        0x0001u
#define IRQ_EP1        0x0002u
#define IRQ_EP2        0x0004u
/* CSR0: EP0's control/status register (TXCSR with INDEX = 0). SVDOUT and
 * SVDSETUP are write-1 commands that clear OUTPKTRDY and SETUPEND. */
#define CSR0_OUTPKTRDY 0x01u
#define CSR0_INPKTRDY  0x02u
#define CSR0_SENTSTALL 0x04u
#define CSR0_DATAEND   0x08u
#define CSR0_SETUPEND  0x10u
#define CSR0_SENDSTALL 0x20u
#define CSR0_SVDOUT    0x40u
#define CSR0_SVDSETUP  0x80u
/* TXCSR (IN) and RXCSR (OUT) bits for EP1/EP2. */
#define INCSR_INPKTRDY     0x0001u  /* packet loaded, controller owns it */
#define INCSR_FIFONOTEMPTY 0x0002u  /* FIFO holds at least one packet */
/* TXCSR bit 2: set at times on hardware, role on this core not established
 * (MUSB calls it UnderRun). Recorded in observed_flags, never fatal. */
#define INCSR_OBSERVED_BIT2 0x0004u
#define INCSR_FLUSHFIFO    0x0008u
#define INCSR_SENDSTALL    0x0010u
#define INCSR_SENTSTALL    0x0020u
#define INCSR_CLEARDT      0x0040u
#define OUTCSR_OUTPKTRDY   0x0001u  /* a received packet waits in the FIFO */
#define OUTCSR_OVERRUN     0x0004u
#define OUTCSR_FLUSHFIFO   0x0010u
#define OUTCSR_SENDSTALL   0x0020u
#define OUTCSR_SENTSTALL   0x0040u
#define OUTCSR_CLEARDT     0x0080u
/* TXCSR bits 15:8 in PIO: only bit 13 set (MUSB: Mode = TX), here called
 * PIO mode; meaning on this core untested. Every TXCSR write includes it;
 * other bits there mean the endpoint was reconfigured. The EP1 DMA path
 * sets AutoSet/DMAReqEn/DMAReqMode (TXCSRH_DMA) and ep1_dma_end() restores
 * plain PIO mode before PIO resumes. */
#define INCSRH_MASK        0xff00u
#define INCSR_PIO_MODE     0x2000u
/* RXCSR high byte: zero in PIO (DMA sets AutoClear/DMAReqEn there). */
#define OUTCSRH_MASK       0xff00u

/* The controller's DMA (Mentor HSDMA), channel 0 (usb-drv-yp3.c). */
#define R_DMA_INTR     0x200u     /* per-channel done bits, clear on read */
#define R_DMA_CTRL     0x204u     /* channel 0 control */
#define R_DMA_ADDR     0x208u     /* channel 0 memory address; advances */
#define R_DMA_COUNT    0x20cu     /* channel 0 byte count */
#define DMA_INTR_CH0   0x01u
#define DMA_CTRL_EN    0x0001u
#define DMA_CTRL_TX    0x0002u    /* direction: memory -> FIFO (IN) */
/* mode 1: one request per packet, whole transfer */
#define DMA_CTRL_MODE1 0x0004u
#define DMA_CTRL_IE    0x0008u
#define DMA_CTRL_EP1   0x0010u    /* endpoint number field (bits 7:4) = 1 */
#define TXCSRH_DMA     0x94u      /* AutoSet, DMAReqEn, DMAReqMode 1 */
#define RXCSRH_DMA     0xa0u      /* AutoClear, DMAReqEn */

#endif
