/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BT_DRV_H
#define BT_DRV_H
/* Bluetooth controller driver: the only interface between the generic host
 * stack (firmware/bluetooth/) and a target. A driver may wrap a UART chip,
 * an on-chip controller or, in the simulator, a host socket.
 *
 * Packets use H4 framing: one indicator byte (BT_H4_*) followed by the HCI
 * packet exactly as in the Core spec (Vol 4, Part E, 5.4), little-endian.
 * One complete packet per call in both directions. The stack obeys HCI
 * command and ACL flow control (Num_HCI_Command_Packets, Read_Buffer_Size,
 * Number_Of_Completed_Packets); the driver must report them honestly. */
#include <stdbool.h>
#include <stddef.h>

/* H4 packet indicators (Core spec Vol 4, Part A) */
#define BT_H4_CMD   0x01    /* HCI command, host to controller */
#define BT_H4_ACL   0x02    /* ACL data, both ways */
#define BT_H4_SCO   0x03    /* synchronous (SCO) data, both ways */
#define BT_H4_EVT   0x04    /* HCI event, controller to host */

/* Receives one H4 packet from the controller. May be called from interrupt
 * context, but from one context at a time (the stack queues packets in a
 * single-producer ring); pkt is only valid during the call. The stack uses
 * events and ACL data of up to 1024 bytes, H4 byte included, and drops
 * anything else. Calls may start before bt_drv_open() returns and end
 * when bt_drv_close() does. */
typedef void (*bt_drv_recv_fn)(const void *pkt, size_t len);

/* Power the controller and radio up and make it ready for HCI_Reset
 * (patches loaded, its address set: the stack only reads it). Called from
 * the Bluetooth thread, which has a small stack; this and the stack's HCI
 * set-up must finish within its power-on wait (15 s). Returns false if
 * bring-up failed (the driver has powered down again). */
bool bt_drv_open(bt_drv_recv_fn recv);

/* Power the controller and radio down. No recv calls after this returns. */
void bt_drv_close(void);

/* Send one H4 packet. The driver has copied it when this returns. Called
 * from the Bluetooth thread and from the tick interrupt (the media path),
 * never both at once, so it must not block. Returns false when the driver
 * is out of room despite flow control: the stack sends the packet again a
 * little later (commands, ACL), so a driver whose queue holds all of the
 * controller's ACL buffers never needs to. */
bool bt_drv_send(const void *pkt, size_t len);

#ifdef HAVE_BLUETOOTH_DRV_CAL
/* Radio calibration a driver can reuse instead of measuring at every
 * bt_drv_open() (YP3: ~5 s). The stack keeps it in a file as an opaque blob
 * of at most BT_DRV_CAL_MAX bytes; the driver validates it. */
#define BT_DRV_CAL_MAX  384

/* Lend the driver a word-aligned buffer of `size` bytes from before
 * bt_drv_open() until after bt_drv_close() (buf NULL ends the loan). It
 * holds `len` bytes of stored calibration (0: none). The driver uses valid
 * data instead of measuring, and puts a fresh calibration there. */
void bt_drv_cal_attach(void *buf, size_t size, size_t len);

/* After bt_drv_open(): the length of a fresh calibration in the buffer, to
 * be stored, or 0 if there is none. Reports each one once. */
size_t bt_drv_cal_fresh(void);
#endif

#endif /* BT_DRV_H */
