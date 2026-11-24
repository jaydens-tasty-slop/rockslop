/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef YP3_USB_TARGET_H
#define YP3_USB_TARGET_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "cpu.h"

/* A successful prepare grants exclusive controller access with IRQ41 masked,
 * IRQ42/DMA and the separate IRQ43 transfer owner retired, and no other
 * callback/timer able to reconnect it. It must establish accessible MMIO and
 * verified clocks/PHY/pins without enabling controller interrupts. */
bool yp3_usb_board_prepare(void);
void yp3_usb_board_release(void);
bool yp3_usb_board_vbus_valid(void);
/* controller IRQ41 and DMA IRQ42 */
void yp3_usb_board_irq_enable(bool enable);
/* IRQ42: the controller's DMA engine (usb-drv-yp3.c). */
void yp3_usb_drv_dma_int(void);

enum yp3_usb_controller_stage {
    YP3_USB_CONTROLLER_DISCONNECTED,
    YP3_USB_CONTROLLER_FLUSHED
};
/* Called with SOFTCONN clear and interrupt sources masked. DISCONNECTED must
 * complete a bounded, accepted disconnect wait and device-mode/speed setup
 * before indexed endpoint writes. FLUSHED must complete the accepted settling
 * boundary before consuming status/enabling sources. The wait time units are
 * not established. Failure retains ownership in quarantine; do not
 * speculate about restoring the previous owner. */
bool yp3_usb_board_controller_stage(enum yp3_usb_controller_stage stage);

/* Controller register access by offset from USB_BASE (sl6801.h). */
#define YP3_USB_READ8(o) (*(volatile uint8_t *)(USB_BASE + (o)))
#define YP3_USB_READ16(o) (*(volatile uint16_t *)(USB_BASE + (o)))
#define YP3_USB_WRITE8(o,v) \
    (*(volatile uint8_t *)(USB_BASE + (o)) = (uint8_t)(v))
#define YP3_USB_WRITE16(o,v) \
    (*(volatile uint16_t *)(USB_BASE + (o)) = (uint16_t)(v))
#define YP3_USB_READ32(o) (*(volatile uint32_t *)(USB_BASE + (o)))
#define YP3_USB_WRITE32(o,v) \
    (*(volatile uint32_t *)(USB_BASE + (o)) = (uint32_t)(v))

#endif
