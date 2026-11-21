/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Full-speed USB device driver: EP0 control, EP1 bulk IN and OUT, EP2
 * interrupt IN. */
#include "config.h"
#include "system.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "usb-target.h"
#include "dev-yp3.h"
#include "recovery-yp3.h"
#ifdef YP3_DEV
struct yp3_usb_stats yp3_usb_stats;
#endif
#include "usb-pio-yp3.h"
#include "usb-regs-yp3.h"
#include <stdio.h>
#include <string.h>

/* Controller registers and bits: usb-regs-yp3.h. */

/* EP1 bulk transfers of whole packets use the controller's DMA (Mentor
 * HSDMA channel 0, IRQ42; ROM 0x53b8): mode 1 with AutoClear/AutoSet moves
 * packets without the CPU, so the host is not NAKed while an IRQ drains the
 * FIFO (PIO managed ~3 packets per 1 ms frame). Other transfers use PIO. */
enum { EP1_DMA_NONE, EP1_DMA_OUT, EP1_DMA_IN };
/* A short OUT packet under DMA raised no interrupt (MSC CBWs into a 512 B
 * buffer hung), so OUT uses DMA only above this: MSC multi-sector data. */
#define EP1_DMA_MIN_OUT 1024u

/* every SETUP packet is 8 bytes (USB 2.0 9.3) */
#define SETUP_PACKET_SIZE 8u
#define SETUP_WLENGTH_LO 6         /* wLength bytes in the SETUP packet */
#define SETUP_WLENGTH_HI 7
#define USB_ADDRESS_MASK 0x7fu     /* device addresses are 7 bits */
#define EP0_MAX_TRANSFER 0xffff    /* wLength is 16 bits */
#define TRANSFER_TIMEOUT (HZ * 5)  /* blocking send/recv give up after 5 s */
/* Bound on CSR polls while waiting for a flush or an EP0 ack to land; an
 * access budget, not a measured silicon time. */
#define FLUSH_POLL_LIMIT 10000u
/* driver_error bits. RETIREMENT: a FIFO could not be flushed or acked, so
 * the driver shuts itself off (interrupt sources masked, every call
 * refused). It is sticky: usb_drv_init refuses while it is set, and only
 * usb_drv_startup() clears it.
 * REQUEST records a rejected API call (bad endpoint/type) and is cleared by
 * the next successful init. */
#define DRIVER_ERROR_RETIREMENT 0x01
#define DRIVER_ERROR_REQUEST    0x02

struct yp3_dir_state {
    struct semaphore done;
    struct yp3_usb_pio_in in;
    struct yp3_usb_pio_out out;
    void *buffer;
    uint32_t length;
    uint32_t progress;
    int status;
    bool active;
    bool waiting;
    bool control_status;
    bool in_flight;
};

static struct yp3_dir_state ep0_in, ep0_out, ep1_in, ep1_out, ep2_in;
static bool driver_ready;
static bool ep2_allocated;
static bool set_address_pending;
static uint8_t pending_address;
static uint16_t setup_wlength;
static bool ep0_zlp_pending;
static uint8_t driver_error;
static uint8_t ep1_dma;          /* EP1_DMA_*: direction DMA owns */
static uintptr_t ep1_dma_start;

static void latch_request_error(void)
{
    /* Thread-mode request validation shares this byte with IRQ retirement.
     * Preserve both the retirement latch and the caller's interrupt mask. */
    int irq = disable_irq_save();
    driver_error |= DRIVER_ERROR_REQUEST;
    restore_irq(irq);
}

struct usb_drv_ep_spec usb_drv_ep_specs[USB_NUM_ENDPOINTS];
uint8_t usb_drv_ep_specs_flags = USB_ENDPOINT_SPEC_FORCE_IO_TYPE_MATCH;

static struct yp3_dir_state * ICODE_ATTR state_for(unsigned ep, bool in)
{
    if (ep == 0) return in ? &ep0_in : &ep0_out;
    if (ep == 1) return in ? &ep1_in : &ep1_out;
    if (ep == 2 && in) return &ep2_in;
    return NULL;
}

static uint8_t ICODE_ATTR index_enter(unsigned ep)
{
    uint8_t old = YP3_USB_READ8(R_INDEX);
    YP3_USB_WRITE8(R_INDEX, (uint8_t)ep);
    return old;
}

static void ICODE_ATTR index_leave(uint8_t old)
{
    YP3_USB_WRITE8(R_INDEX, old);
}

/* With INDEX = 1. The transfer is base[0..length); DMA moves from
 * base + done (an OUT packet already in the FIFO is taken by PIO first). */
static void ICODE_ATTR ep1_dma_begin(bool in, uint8_t *base, uint32_t done,
                                     uint32_t length)
{
    ep1_dma = in ? EP1_DMA_IN : EP1_DMA_OUT;
    ep1_dma_start = (uintptr_t)base;
    if (in)
        YP3_USB_WRITE8(R_TXCSRH, (INCSR_PIO_MODE >> 8) | TXCSRH_DMA);
    else
        YP3_USB_WRITE8(R_RXCSRH, RXCSRH_DMA);
    YP3_USB_WRITE32(R_DMA_ADDR, (uint32_t)(uintptr_t)(base + done));
    YP3_USB_WRITE32(R_DMA_COUNT, length - done);
    YP3_USB_WRITE16(R_DMA_CTRL, DMA_CTRL_EN | DMA_CTRL_MODE1 | DMA_CTRL_IE |
                                DMA_CTRL_EP1 | (in ? DMA_CTRL_TX : 0));
}

/* Stop the channel and return EP1 to PIO; bytes the DMA moved. */
static uint32_t ICODE_ATTR ep1_dma_end(void)
{
    if (ep1_dma == EP1_DMA_NONE)
        return 0;
    YP3_USB_WRITE16(R_DMA_CTRL, 0);
    uint32_t moved = YP3_USB_READ32(R_DMA_ADDR) - (uint32_t)ep1_dma_start;
    uint8_t old = index_enter(1);
    if (ep1_dma == EP1_DMA_IN)
        YP3_USB_WRITE8(R_TXCSRH, INCSR_PIO_MODE >> 8);
    else
        YP3_USB_WRITE8(R_RXCSRH, 0);
    index_leave(old);
    ep1_dma = EP1_DMA_NONE;
    return moved;
}

static void drain_signal(struct yp3_dir_state *s)
{
    (void)semaphore_wait(&s->done, TIMEOUT_NOBLOCK);
}

static void signal_done(struct yp3_dir_state *s, int status)
{
    s->status = status;
    s->active = false;
    if (s->waiting)
        semaphore_release(&s->done);
}

static bool wait_done(struct yp3_dir_state *s)
{
    if (semaphore_wait(&s->done, TRANSFER_TIMEOUT) == OBJ_WAIT_SUCCEEDED)
        return s->status == 0;
    /* Leave the blocking-owner marker set until timeout cleanup discards the
     * request. That cleanup must not publish a second, asynchronous
     * callback. */
    return false;
}

static void retirement_failed(void)
{
    if (driver_error & DRIVER_ERROR_RETIREMENT) return;
    driver_error |= DRIVER_ERROR_RETIREMENT;
    /* Do not service/rearm an uncertain FIFO or leave its IRQ source spinning.
     * Recovery requires a fresh cold startup and board ownership, not a new
     * request or an unrelated successful endpoint flush. */
    YP3_USB_WRITE8(R_INTRTXE, 0);
    YP3_USB_WRITE8(R_INTRRXE, 0);
    YP3_USB_WRITE8(R_INTRUSBE, 0);
    usb_drv_int_enable(false);
}

static bool flush_ep(unsigned ep, bool in)
{
    uint8_t old = index_enter(ep);
    uint16_t command = in ?
        (INCSR_PIO_MODE | INCSR_FLUSHFIFO | INCSR_CLEARDT) :
        (OUTCSR_FLUSHFIFO | OUTCSR_CLEARDT);
    uint16_t reg = in ? R_TXCSR : R_RXCSR;
    /* Written twice (MUSB's recipe for double-buffered FIFOs; the ROM, 0x4cf4/
     * 0x4d14, writes once). Kept as-is; works on hardware. */
    YP3_USB_WRITE16(reg, command);
    YP3_USB_WRITE16(reg, command);
    bool idle = false;
    for (unsigned i = 0; i < FLUSH_POLL_LIMIT; ++i) {
        uint16_t csr = YP3_USB_READ16(reg);
        if (in ? !(csr & (INCSR_INPKTRDY | INCSR_FIFONOTEMPTY)) :
                 !(csr & (OUTCSR_OUTPKTRDY | OUTCSR_OVERRUN))) {
            idle = true;
            break;
        }
    }
    index_leave(old);
    return idle;
}

/* Caller owns INDEX=0. ROM 0x50e8/0x50b8 wait for OUTPKTRDY to clear after
 * SVDOUT.
 * A later packet can reassert READY before it is sampled: do not acknowledge
 * it again here. An ambiguous timeout stops the driver, never reports success.
 * This poll cap is a finite access budget, not a measured silicon deadline. */
static bool ep0_ack_out(uint16_t command)
{
    YP3_USB_WRITE16(R_TXCSR, command);
    if (!(command & CSR0_SVDOUT)) return true;
    for (unsigned i = 0; i < FLUSH_POLL_LIMIT; ++i)
        if (!(YP3_USB_READ16(R_TXCSR) & CSR0_OUTPKTRDY)) return true;
    retirement_failed();
    return false;
}

static bool flush_ep0(uint16_t csr)
{
    uint8_t old = index_enter(0);
    /* The IRQ owner supplies its classified snapshot. A fresh status sample
     * here could acknowledge a SETUP that arrived after that
     * classification. */
    uint16_t command = 0;
    if (csr & CSR0_OUTPKTRDY) command |= CSR0_SVDOUT;
    if (csr & CSR0_SETUPEND) command |= CSR0_SVDSETUP;
    bool ok = !command || ep0_ack_out(command);
    index_leave(old);
    return ok; /* No separate EP0 FIFO-flush bit is established. */
}

static void discard_state(struct yp3_dir_state *s, bool wake)
{
    if ((s == &ep1_in && ep1_dma == EP1_DMA_IN) ||
        (s == &ep1_out && ep1_dma == EP1_DMA_OUT))
        (void)ep1_dma_end();
    bool waiter = s->waiting;
    s->active = false;
    s->waiting = false;
    s->status = -1;
    s->control_status = false;
    memset(&s->in, 0, sizeof(s->in));
    memset(&s->out, 0, sizeof(s->out));
    s->in_flight = false;
    if (wake && waiter) semaphore_release(&s->done);
    drain_signal(s);
}

static void discard_ep0(void)
{
    discard_state(&ep0_in, true);
    discard_state(&ep0_out, true);
    set_address_pending = false;
    ep0_zlp_pending = false;
}

static void cancel_ep0(uint16_t csr)
{
    discard_ep0();
    (void)flush_ep0(csr);
}

/* Drop the EP1/EP2 requests and flush their FIFOs; a FIFO that will not
 * flush retires the driver. False if the driver was retired already (no
 * flush tried). Runs in the USB IRQ or with interrupts masked, so that no
 * completion lands on a request being dropped. */
static bool reset_data_endpoints(void)
{
    discard_state(&ep1_in, true); discard_state(&ep1_out, true);
    discard_state(&ep2_in, true);
    if (driver_error & DRIVER_ERROR_RETIREMENT) return false;
    bool ok_in = flush_ep(1,true), ok_out = flush_ep(1,false);
    if (!flush_ep(2,true)) ok_in = false;
    if (!ok_in || !ok_out) retirement_failed();
    return true;
}

/* Core cancellation/reset/timeout does not own the currently pending packet.
 * An inactive request (e.g. after SETUP retirement) needs only software
 * discard. Active ownership has no proven generic EP0 FIFO retirement
 * command: wake the waiter and quarantine instead of acknowledging/reusing
 * an unrelated packet. */
static void cancel_ep0_request(void)
{
    int old = disable_irq_save();
    bool active = ep0_in.active || ep0_out.active;
    discard_ep0();
    if (active) retirement_failed();
    restore_irq(old);
}

/* SETUP packet layout (USB 2.0 9.3): byte fields, then three 16-bit
 * little-endian fields. */
static bool setup_request(uint8_t *bytes, struct usb_ctrlrequest *req)
{
    req->bRequestType = bytes[0];
    req->bRequest = bytes[1];
    req->wValue = (uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8);
    req->wIndex = (uint16_t)bytes[4] | ((uint16_t)bytes[5] << 8);
    req->wLength = (uint16_t)bytes[6] | ((uint16_t)bytes[7] << 8);
    return true;
}

/* MUSB handles EP0 status stages in hardware: the zero-length OUT that
 * usb_core arms for a control-IN status never produces a packet. Only an
 * armed OUT data stage owns OUTPKTRDY; anything else is a SETUP. */
static bool ep0_out_data_expected(void)
{
    return ep0_out.active && ep0_out.length != 0;
}

/* SETUP of a request without data stage, not yet acknowledged (see
 * setup_irq). */
static bool ep0_setup_ack_pending;

static bool queue_ep0_packet(struct yp3_dir_state *s)
{
    uint32_t remain = s->length - s->progress;
    unsigned count = remain > YP3_USB_MPS ? YP3_USB_MPS : (unsigned)remain;
    if (!remain && !ep0_zlp_pending && s->length != 0)
        return false;
    uint8_t old = index_enter(0);
    if (count)
        yp3_usb_pio_fifo_write(0, (const uint8_t *)s->buffer + s->progress,
                               count);
    s->progress += count;
    /* A request without data stage (s->length == 0) completes its status
     * stage with DATAEND alone; INPKTRDY there makes the controller report
     * SETUPEND and the request (e.g. SET_ADDRESS) never completes. */
    uint8_t command = s->length == 0 ? 0 : CSR0_INPKTRDY;
    if (s->length == 0 && ep0_setup_ack_pending) {
        command |= CSR0_SVDOUT;
        ep0_setup_ack_pending = false;
    }
    if (s->progress == s->length) {
        if (ep0_zlp_pending && count != 0) {
            /* The host requested more than the short exact-multiple reply. */
        } else {
            if (ep0_zlp_pending) ep0_zlp_pending = false;
            command |= CSR0_DATAEND;
        }
    }
    YP3_USB_WRITE16(R_TXCSR, command);
    index_leave(old);
    s->control_status = (count == 0);
    return true;
}

static void start_ep0_in(void *buffer, uint32_t length, bool address_status)
{
    struct yp3_dir_state *s = &ep0_in;
    drain_signal(s);
    s->buffer = buffer;
    s->length = length;
    s->progress = 0;
    s->status = 0;
    s->active = true;
    s->waiting = false;
    s->control_status = false;
    s->in_flight = 0;
    ep0_zlp_pending = address_status ? false :
        (length != 0 && length < setup_wlength && (length % YP3_USB_MPS) == 0);
    set_address_pending = address_status;
    queue_ep0_packet(s);
    s->in_flight = 1;
}

static void ICODE_ATTR complete_dir(unsigned ep, bool in,
                                    struct yp3_dir_state *s, int status,
                                    uint32_t count)
{
    signal_done(s, status);
    if (ep == 0 && in && set_address_pending) {
        if (status == 0) {
            YP3_USB_WRITE8(R_FADDR, pending_address);
            usb_core_notify_set_address(pending_address);
        }
        set_address_pending = false;
        return;
    }
    /* usb_core_transfer_complete() consumes USB token direction bits, not the
     * endpoint-allocation DIR_IN/DIR_OUT indexes. */
    usb_core_transfer_complete((int)ep, in ? USB_DIR_IN : USB_DIR_OUT,
                               status, (int)count);
}

static void setup_irq(uint16_t csr)
{
    uint8_t old = index_enter(0);
    /* ROM 0x6ee0 services SETUPEND before reading the replacement FIFO. Keep
     * command-only writes; sampled-CSR replay is still a physical gate. */
    if (csr & CSR0_SETUPEND) YP3_USB_WRITE16(R_TXCSR, CSR0_SVDSETUP);
    unsigned count = YP3_USB_READ16(R_RXCOUNT);
    if (count > YP3_USB_MPS) count = YP3_USB_MPS;   /* never overrun bytes[] */
    uint8_t bytes[YP3_USB_MPS];
    yp3_usb_pio_fifo_read(0, bytes, count, count);
    /* MUSB: a request without data stage is acknowledged together with
     * DATAEND (or SENDSTALL) when the reply is known; acking it alone first
     * ends the transfer with SETUPEND. */
    bool no_data = count == SETUP_PACKET_SIZE &&
                   bytes[SETUP_WLENGTH_LO] == 0 &&
                   bytes[SETUP_WLENGTH_HI] == 0;
    bool retired = no_data || ep0_ack_out(CSR0_SVDOUT);
    ep0_setup_ack_pending = no_data;
    index_leave(old);

    /* Discard both directions before making the replacement visible to
     * core. */
    /* The packet was already acknowledged. Do not sample/ack a later packet
     * while discarding the old software request. */
    discard_ep0();
    usb_core_control_cancelled();
    if (!retired) return;
    if (count != SETUP_PACKET_SIZE) {
        usb_drv_stall(EP_CONTROL, true, true);
        return;
    }
    struct usb_ctrlrequest req;
    setup_request(bytes, &req);
    if (usb_drv_is_set_address(&req)) {
        pending_address = (uint8_t)(req.wValue & USB_ADDRESS_MASK);
        start_ep0_in(NULL, 0, true);
    } else {
        setup_wlength = req.wLength;
        usb_core_setup_received(&req);
    }
}

static void ep0_out_irq(void)
{
    struct yp3_dir_state *s = &ep0_out;
    uint8_t old = index_enter(0);
    uint16_t csr = YP3_USB_READ16(R_TXCSR);
    unsigned count = YP3_USB_READ16(R_RXCOUNT);
    if (!(csr & CSR0_OUTPKTRDY)) { index_leave(old); return; }
    /* A SETUP supersedes an OUT data/status request. SETUPEND is the family
     * indication; the idle receiver plus the mandatory eight-byte packet is
     * the initial-setup case. */
    if ((csr & CSR0_SETUPEND) ||
        ((!ep0_out_data_expected() || (csr & CSR0_SENTSTALL)) &&
         count == SETUP_PACKET_SIZE)) {
        index_leave(old);
        setup_irq(csr);
        return;
    }
    if (!s->active) {
        unsigned drain = count < YP3_USB_MPS ? count : YP3_USB_MPS;
        yp3_usb_pio_fifo_read(0, NULL, 0, drain);
        (void)ep0_ack_out(CSR0_SVDOUT);
        index_leave(old);
        return;
    }
    if (count > YP3_USB_MPS) {
        yp3_usb_pio_fifo_read(0, NULL, 0, YP3_USB_MPS);
        (void)ep0_ack_out(CSR0_SVDOUT);
        index_leave(old);
        complete_dir(0, false, s, -1, s->progress);
        return;
    }
    uint32_t remain = s->length - s->progress;
    unsigned copy = count < remain ? count : (unsigned)remain;
    uint8_t *dst = s->buffer ? (uint8_t *)s->buffer + s->progress : NULL;
    yp3_usb_pio_fifo_read(0, dst, copy, count);
    s->progress += copy;
    bool retired = ep0_ack_out(CSR0_SVDOUT);
    index_leave(old);
    if (!retired || count > remain ||
        (count < YP3_USB_MPS && s->progress < s->length))
        complete_dir(0, false, s, -1, s->progress);
    else if (s->progress == s->length)
        complete_dir(0, false, s, 0, s->progress);
}

static void ep0_in_irq(void)
{
    if (driver_error & DRIVER_ERROR_RETIREMENT) return;
    struct yp3_dir_state *s = &ep0_in;
    if (!s->active || !s->in_flight) return;
    uint8_t old = index_enter(0);
    uint16_t csr = YP3_USB_READ16(R_TXCSR);
    index_leave(old);
    if (csr & CSR0_INPKTRDY) return;
    s->in_flight = 0;
    if (s->progress < s->length || ep0_zlp_pending) {
        queue_ep0_packet(s);
        s->in_flight = 1;
        return;
    }
    uint32_t sent = s->length;
    s->active = false;
    if (set_address_pending) {
        complete_dir(0, true, s, 0, 0);
    } else {
        complete_dir(0, true, s, 0, sent);
        /* The host's status OUT was handled by the controller (DATAEND). */
        if (ep0_out.active && ep0_out.length == 0) {
            ep0_out.active = false;
            complete_dir(0, false, &ep0_out, 0, 0);
        }
    }
}

static void ICODE_ATTR endpoint_irq(unsigned ep, bool in)
{
    struct yp3_dir_state *s = state_for(ep, in);
    if (!s || !s->active) return;
    if (ep == 1 && ep1_dma == (in ? EP1_DMA_IN : EP1_DMA_OUT)) {
        /* IN: packets of a DMA transfer; the DMA IRQ completes it. */
        if (in) return;
        /* OUT: a short packet ended the transfer early. PIO takes it from
         * where the DMA stopped. */
        uint32_t moved = ep1_dma_end();
        s->out.buffer = (uint8_t *)ep1_dma_start;
        s->out.capacity = s->length;
        s->out.received = moved;
        s->out.active = 1;
    }
    uint8_t old = index_enter(ep);
    enum yp3_usb_pio_result result = in ? yp3_usb_pio_in_irq(&s->in) :
                                          yp3_usb_pio_out_irq(&s->out);
    index_leave(old);
    if (result == YP3_USB_PIO_ACTIVE) return;
    if (result == YP3_USB_PIO_DONE || result == YP3_USB_PIO_SHORT ||
        result == YP3_USB_PIO_ZLP) {
        uint32_t count = in ? s->length : s->out.received;
        complete_dir(ep, in, s, 0, count);
    } else {
        uint32_t count = in ? s->in.queued : s->out.received;
        if (!in && (result == YP3_USB_PIO_BAD_COUNT ||
                    result == YP3_USB_PIO_OVERFLOW) &&
            !flush_ep(ep, false))
            retirement_failed();
        complete_dir(ep, in, s, -1, count);
    }
}

void usb_drv_startup(void)
{
    /* Cold core startup only; this is not a warm retirement recovery API. */
    driver_ready = false;
    driver_error = 0;
    ep2_allocated = false;
    for (unsigned i = 0; i < USB_NUM_ENDPOINTS; ++i) {
        usb_drv_ep_specs[i].type[DIR_IN] = USB_ENDPOINT_TYPE_NONE;
        usb_drv_ep_specs[i].type[DIR_OUT] = USB_ENDPOINT_TYPE_NONE;
    }
    usb_drv_ep_specs[0].type[DIR_IN] = USB_ENDPOINT_XFER_CONTROL;
    usb_drv_ep_specs[0].type[DIR_OUT] = USB_ENDPOINT_XFER_CONTROL;
#if USB_NUM_ENDPOINTS >= 2
    usb_drv_ep_specs[1].type[DIR_IN] = USB_ENDPOINT_XFER_BULK;
    usb_drv_ep_specs[1].type[DIR_OUT] = USB_ENDPOINT_XFER_BULK;
#endif
#if USB_NUM_ENDPOINTS >= 3
    usb_drv_ep_specs[2].type[DIR_IN] = USB_ENDPOINT_XFER_INT;
#endif
    semaphore_init(&ep0_in.done, 1, 0); semaphore_init(&ep0_out.done, 1, 0);
    semaphore_init(&ep1_in.done, 1, 0); semaphore_init(&ep1_out.done, 1, 0);
    semaphore_init(&ep2_in.done, 1, 0);
}

void usb_drv_init(void)
{
    if (driver_ready || (driver_error & DRIVER_ERROR_RETIREMENT)) return;
    if (!yp3_usb_board_prepare()) {
        driver_ready = false;
        latch_request_error();
        return;
    }
    driver_error = 0;
    usb_drv_int_enable(false);
    YP3_USB_WRITE8(R_INTRTXE, 0);
    YP3_USB_WRITE8(R_INTRRXE, 0);
    YP3_USB_WRITE8(R_INTRUSBE, 0);
    /* ROM 0x5524 first disconnects (via 0x5204) before mode/endpoint setup.
     * Preserve the other POWER bits, and let the board qualify timing and
     * device mode. */
    YP3_USB_WRITE8(USB_REG_POWER,
                   (uint8_t)(YP3_USB_READ8(USB_REG_POWER) &
                             ~USB_POWER_SOFTCONN));
    if (!yp3_usb_board_controller_stage(YP3_USB_CONTROLLER_DISCONNECTED)) {
        retirement_failed();
        return;
    }
    discard_state(&ep0_in, false); discard_state(&ep0_out, false);
    discard_state(&ep1_in, false); discard_state(&ep1_out, false);
    discard_state(&ep2_in, false);
    uint8_t old = index_enter(0);
    YP3_USB_WRITE16(R_TXMAXP, YP3_USB_MPS);
    YP3_USB_WRITE16(R_RXMAXP, YP3_USB_MPS);
    if (!ep0_ack_out(CSR0_SVDOUT | CSR0_SVDSETUP)) {
        index_leave(old);
        return;
    }
    index_leave(old);
    old = index_enter(1);
    YP3_USB_WRITE16(R_TXMAXP, YP3_USB_MPS);
    YP3_USB_WRITE16(R_RXMAXP, YP3_USB_MPS);
    index_leave(old);
    /* Keep sources masked until the bounded initial retirement succeeds. */
    if (!flush_ep(1,true) || !flush_ep(1,false) || !flush_ep(2,true)) {
        retirement_failed();
        return;
    }
    if (!yp3_usb_board_controller_stage(YP3_USB_CONTROLLER_FLUSHED)) {
        retirement_failed();
        return;
    }
    /* ROM 0x4d6a drains each clear-on-read status byte before enabling
     * sources. */
    (void)YP3_USB_READ8(R_INTRUSB);
    (void)YP3_USB_READ8(R_INTRRX);
    (void)YP3_USB_READ8(R_INTRTX);
    /* EP2 only after allocation */
    YP3_USB_WRITE8(R_INTRTXE, (uint8_t)(IRQ_EP0 | IRQ_EP1));
    /* EP0 uses TX status for both directions */
    YP3_USB_WRITE8(R_INTRRXE, (uint8_t)IRQ_EP1);
    YP3_USB_WRITE8(R_INTRUSBE, INTRUSB_RESET); /* no SOF/DMA/suspend enables */
    driver_ready = true;
    usb_drv_int_enable(true);
    usb_attach();   /* USB_DETECT_BY_REQUEST: the host's requests decide */
}

void usb_drv_exit(void)
{
    bool was_ready = driver_ready;
    if (!was_ready) return;
    YP3_USB_WRITE8(R_INTRTXE, 0);
    YP3_USB_WRITE8(R_INTRRXE, 0);
    YP3_USB_WRITE8(R_INTRUSBE, 0);
    usb_drv_int_enable(false);
    yp3_usb_drop_softconn();
    /* Unplugged mid-transfer, EP0 can still own a request. The controller
     * is going down, so drop it in software alone: cancel_ep0_request()
     * would retire the driver, which keeps the board powered and refuses
     * every later init until a reboot. */
    int irq = disable_irq_save();
    discard_ep0();
    restore_irq(irq);
    usb_drv_cancel_all_transfers();
    discard_state(&ep0_in, true); discard_state(&ep0_out, true);
    discard_state(&ep1_in, true); discard_state(&ep1_out, true);
    discard_state(&ep2_in, true);
    driver_ready = false;
    if (!(driver_error & DRIVER_ERROR_RETIREMENT)) yp3_usb_board_release();
}

void usb_attach(void)
{
    if (driver_ready && !(driver_error & DRIVER_ERROR_RETIREMENT)) {
        yp3_usb_softconn = 1;
        YP3_USB_WRITE8(USB_REG_POWER,
                       (uint8_t)(YP3_USB_READ8(USB_REG_POWER) |
                                 USB_POWER_SOFTCONN));
    }
}

void usb_drv_int_enable(bool enable) { yp3_usb_board_irq_enable(enable); }


void ICODE_ATTR yp3_usb_drv_dma_int(void)
{
    uint8_t status = YP3_USB_READ8(R_DMA_INTR);    /* consuming */
    if (!(status & DMA_INTR_CH0) || ep1_dma == EP1_DMA_NONE)
        return;
    bool in = ep1_dma == EP1_DMA_IN;
    struct yp3_dir_state *s = in ? &ep1_in : &ep1_out;
    uint32_t moved = ep1_dma_end();
    if (!s->active)
        return;
    if (!in) {
        complete_dir(1, false, s, moved == s->length ? 0 : -1, moved);
        return;
    }
    /* All data is in the FIFO; complete once the last packet has gone
     * (now, or on its TX interrupt through the PIO state). */
    s->in.buffer = s->buffer;
    s->in.length = s->in.queued = moved;
    s->in.endpoint = 1;
    s->in.append_zlp = s->in.zlp_queued = 0;
    s->in.in_flight = s->in.active = 1;
    s->in.result = YP3_USB_PIO_ACTIVE;
    endpoint_irq(1, true);
}

#ifdef YP3_DEV
void yp3_usb_dev_in_state(char *buf, size_t size)
{
    int old = disable_irq_save();
    struct yp3_dir_state s = ep1_in;
    uint8_t index = index_enter(1);
    uint16_t csr = YP3_USB_READ16(R_TXCSR);
    index_leave(index);
    uint16_t dma_ctrl = YP3_USB_READ16(R_DMA_CTRL);
    uint32_t dma_count = YP3_USB_READ32(R_DMA_COUNT);
    uint8_t dma = ep1_dma, error = driver_error;
    restore_irq(old);
    snprintf(buf, size, "ep1 in active %d len %lu pio %d/%d queued %lu "
             "result %d csr %04x, dma %d ctrl %04x count %lu, error %x, "
             "irqs %lu tx %lu", s.active, (unsigned long)s.length,
             s.in.active, s.in.in_flight, (unsigned long)s.in.queued,
             (int)s.in.result, csr, dma, dma_ctrl, (unsigned long)dma_count,
             error, (unsigned long)yp3_usb_stats.irqs,
             (unsigned long)yp3_usb_stats.src[1]);
}
#endif

void ICODE_ATTR usb_drv_int(void)
{
#ifdef YP3_DEV
    yp3_usb_stats.irqs++;
    yp3_usb_stats.error = driver_error;
#endif
    if (!driver_ready || (driver_error & DRIVER_ERROR_RETIREMENT)) return;
    /* These status reads are consuming on this controller family: take each
     * exactly once, then process reset/setup before any stale endpoint
     * bits. */
    uint8_t global = YP3_USB_READ8(R_INTRUSB);
    uint8_t rx = YP3_USB_READ8(R_INTRRX);
    uint8_t tx = YP3_USB_READ8(R_INTRTX);
#ifdef YP3_DEV
    if (global & INTRUSB_RESET) yp3_usb_stats.resets++;
    if (tx & IRQ_EP0) yp3_usb_stats.ep0++;
    yp3_usb_stats.last = global | (rx << 8) | (tx << 16);
    if (rx & IRQ_EP1) yp3_usb_stats.src[0]++;
    if (tx & IRQ_EP1) yp3_usb_stats.src[1]++;
    if (tx & IRQ_EP0) yp3_usb_stats.src[2]++;
    if (!(rx & IRQ_EP1) && !(tx & (IRQ_EP0 | IRQ_EP1))) yp3_usb_stats.src[3]++;
#endif
    if (global & INTRUSB_RESET) {
        /* Bus reset owns this reset-time snapshot separately from EP0 IRQ. */
        uint8_t old = index_enter(0);
        uint16_t csr = YP3_USB_READ16(R_TXCSR);
        index_leave(old);
        cancel_ep0(csr);
        if (reset_data_endpoints())
            YP3_USB_WRITE8(R_FADDR, 0);
        usb_core_bus_reset();
        return;
    }
    if (tx & IRQ_EP0) {
        uint8_t old = index_enter(0);
        uint16_t csr = YP3_USB_READ16(R_TXCSR);
        bool ready = (csr & CSR0_OUTPKTRDY) != 0;
        bool setup = ready && ((csr & CSR0_SETUPEND) ||
                     ((!ep0_out_data_expected() || (csr & CSR0_SENTSTALL)) &&
                      YP3_USB_READ16(R_RXCOUNT) == SETUP_PACKET_SIZE));
        index_leave(old);
        if (setup) setup_irq(csr);
        else if (csr & (CSR0_SETUPEND | CSR0_SENTSTALL)) {
            cancel_ep0(csr);
            usb_core_control_cancelled();
        } else if (ready) {
            ep0_out_irq();
            /* Last IN ACK and OUT status can coalesce into this same event. */
            ep0_in_irq();
        }
        else ep0_in_irq();
    }
    if (driver_error & DRIVER_ERROR_RETIREMENT) return;
    if (rx & IRQ_EP1) endpoint_irq(1,false);
    if (driver_error & DRIVER_ERROR_RETIREMENT) return;
    if (tx & IRQ_EP1) endpoint_irq(1,true);
    if (tx & IRQ_EP2) endpoint_irq(2,true);
}

static int start_transfer(int endpoint, void *ptr, int length, bool in,
                          bool blocking)
{
    if (!driver_ready || (driver_error & DRIVER_ERROR_RETIREMENT) ||
        length < 0 || (length && !ptr)) return -1;
    unsigned ep = (unsigned)EP_NUM(endpoint);
    if (ep > 2 || (ep == 2 && !in) ||
        (ep == 0 && length > EP0_MAX_TRANSFER))
        return -1;
    if (ep == 2 && !ep2_allocated) return -1;
    struct yp3_dir_state *s = state_for(ep, in);
    if (!s || s->active) return -1;
    int old = disable_irq_save();
    if (!driver_ready || (driver_error & DRIVER_ERROR_RETIREMENT) ||
        s->active || (ep == 2 && !ep2_allocated)) {
        restore_irq(old); return -1;
    }
    drain_signal(s);
    s->buffer = ptr; s->length = (uint32_t)length; s->progress = 0;
    s->status = 0; s->waiting = blocking; s->active = true;
    s->control_status = (ep == 0 && length == 0);
    if (ep == 0 && in)
        ep0_zlp_pending = length != 0 && (uint32_t)length < setup_wlength &&
                          ((unsigned)length % YP3_USB_MPS) == 0;
    enum yp3_usb_pio_result r;
    uint8_t index = index_enter(ep);
    /* DMA: EP1 only, whole packets, a word-aligned buffer, and (OUT) large
     * enough not to end on an unannounced short packet. */
    bool dma = ep == 1 && length && !(length % YP3_USB_MPS) &&
               !((uintptr_t)ptr & 3u) &&
               (in || (uint32_t)length >= EP1_DMA_MIN_OUT) &&
               ep1_dma == EP1_DMA_NONE;
    uint32_t done = 0;
    /* DMA only from an idle FIFO: a queued PIO packet would interleave. */
    if (dma && in &&
        (YP3_USB_READ16(R_TXCSR) & (INCSR_INPKTRDY | INCSR_FIFONOTEMPTY)))
        dma = false;
    if (dma && !in && (YP3_USB_READ16(R_RXCSR) & OUTCSR_OUTPKTRDY)) {
        /* The host is usually quicker than the arming: take the waiting
         * packet by PIO (it must be a full one), DMA the rest. */
        if (YP3_USB_READ16(R_RXCOUNT) == YP3_USB_MPS) {
            yp3_usb_pio_fifo_read(1, ptr, YP3_USB_MPS, YP3_USB_MPS);
            YP3_USB_WRITE16(R_RXCSR, 0);
            done = YP3_USB_MPS;
        } else {
            dma = false;
        }
    }
    if (dma) {
        ep1_dma_begin(in, ptr, done, (uint32_t)length);
        r = YP3_USB_PIO_ACTIVE;
    } else if (ep >= 1 && in) {
        r = yp3_usb_pio_in_start_ep(&s->in, ep, ptr, (uint32_t)length, false);
    } else if (ep == 1) {
        r = yp3_usb_pio_out_start(&s->out, ptr, (uint32_t)length);
    } else if (in) {
        queue_ep0_packet(s); s->in_flight = 1; r = YP3_USB_PIO_ACTIVE;
    } else {
        r = YP3_USB_PIO_ACTIVE; /* EP0 OUT is armed by the next host packet. */
    }
    index_leave(index);
    if (r != YP3_USB_PIO_ACTIVE) {
        s->active = false; s->waiting = false; s->status = -1;
        restore_irq(old); return -1;
    }
    /* Status of a control-IN request without data stage: ack the SETUP with
     * DATAEND; the controller completes the status stage itself. */
    if (ep == 0 && !in && length == 0 && setup_wlength == 0) {
        uint8_t i0 = index_enter(0);
        if (ep0_setup_ack_pending) {
            YP3_USB_WRITE16(R_TXCSR, CSR0_SVDOUT | CSR0_DATAEND);
            ep0_setup_ack_pending = false;
        }
        index_leave(i0);
        s->active = false;
        complete_dir(0, false, s, 0, 0);
    }
    /* An OUT packet that arrived while nothing was armed raised its IRQ
     * already and still waits in the FIFO: service it now. */
    if (ep == 1 && !in && !dma) {
        uint8_t i1 = index_enter(1);
        bool pending = YP3_USB_READ16(R_RXCSR) & OUTCSR_OUTPKTRDY;
        index_leave(i1);
        if (pending) {
            i1 = index_enter(1);
            endpoint_irq(1, false);
            index_leave(i1);
        }
    }
    restore_irq(old);
    if (blocking && !wait_done(s)) {
        if (ep == 0)
            cancel_ep0_request();
        else
            usb_drv_reset_endpoint(endpoint, in);
        return -1;
    }
    return 0;
}

int usb_drv_send(int endpoint, void *ptr, int length)
{ return start_transfer(endpoint, ptr, length, true, true); }
int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{ return start_transfer(endpoint, ptr, length, true, false); }
int usb_drv_recv_blocking(int endpoint, void *ptr, int length)
{ return start_transfer(endpoint, ptr, length, false, true); }
int usb_drv_recv_nonblocking(int endpoint, void *ptr, int length)
{ return start_transfer(endpoint, ptr, length, false, false); }

void usb_drv_cancel_all_transfers(void)
{
    cancel_ep0_request();
    int irq = disable_irq_save();
    (void)reset_data_endpoints();
    restore_irq(irq);
}

void usb_drv_reset_endpoint(int endpoint, bool send)
{
    unsigned ep = (unsigned)EP_NUM(endpoint);
    if (ep > 2 || (ep == 2 && !send)) { latch_request_error(); return; }
    if (ep == 0) { cancel_ep0_request(); return; }
    struct yp3_dir_state *s = state_for(ep, send);
    /* Snapshot and retire one owner atomically against endpoint completion.
     * Otherwise an IRQ can publish success after notify was saved, followed
     * by a second error completion from this cancellation. */
    int irq = disable_irq_save();
    bool notify = ep != 0 && s->active && !s->waiting;
    uint32_t count = send ? s->in.queued : s->out.received;
    discard_state(s, true);
    bool ok = !(driver_error & DRIVER_ERROR_RETIREMENT) && flush_ep(ep,send);
    if (!ok) retirement_failed();
    restore_irq(irq);
    /* Retire software ownership before notifying core. A failed/uncertain
     * hardware flush is still an error completion; it is never success. */
    if (notify)
        usb_core_transfer_complete((int)ep, send ? USB_DIR_IN : USB_DIR_OUT,
                                   -1, (int)count);
}

void usb_drv_ep_init(const struct usb_drv_ep_alloc_ctx *ctx, int endpoint)
{
    (void)ctx;
    unsigned ep = (unsigned)EP_NUM(endpoint);
    if (ep > 2 || !driver_ready || (driver_error & DRIVER_ERROR_RETIREMENT)) {
        latch_request_error(); return;
    }
    int type = ctx ? ctx->type[ep][EP_DIR(endpoint)] : USB_ENDPOINT_TYPE_NONE;
    int mps = ctx ? ctx->max_packet_size[ep][EP_DIR(endpoint)] : -1;
    if ((ep == 2 && (EP_DIR(endpoint) != DIR_IN ||
                     type != USB_ENDPOINT_XFER_INT)) ||
        (ep == 1 && type != USB_ENDPOINT_XFER_BULK) ||
        (ep == 0 && type != USB_ENDPOINT_XFER_CONTROL)) {
        latch_request_error();
        return;
    }
    if (mps <= 0) mps = YP3_USB_MPS;
    if (ep != 0 && mps != YP3_USB_MPS) { latch_request_error(); return; }
    uint8_t old = index_enter(ep);
    if (ep == 0) {
        YP3_USB_WRITE16(R_TXMAXP, YP3_USB_MPS); YP3_USB_WRITE16(R_RXMAXP, YP3_USB_MPS);
    } else {
        if (EP_DIR(endpoint) == DIR_IN)
            YP3_USB_WRITE16(R_TXMAXP, (uint16_t)mps);
        else
            YP3_USB_WRITE16(R_RXMAXP, (uint16_t)mps);
        if (ep == 2) {
            if (!flush_ep(2,true)) {
                retirement_failed();
                index_leave(old);
                return;
            }
            uint8_t mask = YP3_USB_READ8(R_INTRTXE);
            YP3_USB_WRITE8(R_INTRTXE, (uint8_t)(mask | IRQ_EP2));
            ep2_allocated = true;
        }
    }
    index_leave(old);
}
void usb_drv_ep_deinit(const struct usb_drv_ep_alloc_ctx *ctx, int endpoint)
{
    (void)ctx;
    unsigned ep=(unsigned)EP_NUM(endpoint);
    bool in=EP_DIR(endpoint)==DIR_IN;
    if(ep==2 && in) ep2_allocated=false;
    usb_drv_reset_endpoint(endpoint,in);
    if (driver_error & DRIVER_ERROR_RETIREMENT) return;
    if(ep==2 && in) {
        uint8_t mask=YP3_USB_READ8(R_INTRTXE);
        YP3_USB_WRITE8(R_INTRTXE,(uint8_t)(mask&~IRQ_EP2));
    }
}

bool usb_drv_stalled(int endpoint, bool in)
{
    unsigned ep = (unsigned)EP_NUM(endpoint);
    if (ep > 2 || (ep == 2 && !in) ||
        (driver_error & DRIVER_ERROR_RETIREMENT)) return false;
    uint8_t old = index_enter(ep);
    uint16_t csr = YP3_USB_READ16(ep == 0 || in ? R_TXCSR : R_RXCSR);
    index_leave(old);
    return (csr & (ep == 0 ? (CSR0_SENDSTALL | CSR0_SENTSTALL) :
                            (in ? INCSR_SENDSTALL : OUTCSR_SENDSTALL))) != 0;
}
void usb_drv_stall(int endpoint, bool stall, bool in)
{
    unsigned ep = (unsigned)EP_NUM(endpoint);
    if (ep > 2 || (ep == 2 && !in)) { latch_request_error(); return; }
    if (driver_error & DRIVER_ERROR_RETIREMENT) return;
    if (ep == 0) {
        if (stall) cancel_ep0_request();
        if (driver_error & DRIVER_ERROR_RETIREMENT) return;
        uint8_t old = index_enter(0);
        /* Do not consume READY/SETUPEND when changing halt state. Writing zero
         * to clear SENDSTALL is a candidate CSR rule, still a physical gate.
         */
        uint16_t command = stall ? CSR0_SENDSTALL : 0;
        if (stall && ep0_setup_ack_pending) {
            command |= CSR0_SVDOUT;
            ep0_setup_ack_pending = false;
        }
        YP3_USB_WRITE16(R_TXCSR, command);
        index_leave(old);
    } else {
        int irq = disable_irq_save();
        if (driver_error & DRIVER_ERROR_RETIREMENT) {
            restore_irq(irq);
            return;
        }
        struct yp3_dir_state *s = state_for(ep,in);
        bool notify = s->active && !s->waiting;
        uint32_t count = in ? s->in.queued : s->out.received;
        discard_state(s,true);
        bool ok = flush_ep(ep,in);
        if (!ok) retirement_failed();
        else {
            uint8_t old = index_enter(ep);
            uint16_t csr = in ? R_TXCSR : R_RXCSR;
            uint16_t command = stall ?
                (in ? INCSR_PIO_MODE | INCSR_SENDSTALL : OUTCSR_SENDSTALL) :
                (in ? INCSR_PIO_MODE | INCSR_CLEARDT : OUTCSR_CLEARDT);
            YP3_USB_WRITE16(csr, command);
            index_leave(old);
        }
        restore_irq(irq);
        if (notify)
            usb_core_transfer_complete((int)ep, in ? USB_DIR_IN : USB_DIR_OUT,
                                       -1, (int)count);
    }
}

bool usb_drv_powered(void)
{ return driver_ready && yp3_usb_board_vbus_valid(); }
bool usb_drv_connected(void)
{
    return driver_ready &&
           (YP3_USB_READ8(USB_REG_POWER) & USB_POWER_SOFTCONN) != 0;
}
/* this profile is full-speed only */
int usb_drv_port_speed(void) { return 0; }
void usb_drv_set_test_mode(int mode)
{ (void)mode; latch_request_error(); }
