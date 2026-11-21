/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Bluetooth stack: power, thread, RX ring, public API, paired devices.
 *
 * bt_power(true) allocates struct bt_state, starts the thread and waits
 * until the controller is set up (hci.c). The thread then serves API
 * requests (BT_MSG_*), packets that the driver callback queued in the RX
 * ring, and its timers. Paired devices with their link keys live in
 * KEYS_FILE, rewritten when they change; a driver's radio calibration
 * (HAVE_BLUETOOTH_DRV_CAL) lives in CAL_FILE. */
#include "bt.h"
#include <stdio.h>
#include <string.h>
#include "core_alloc.h"
#include "core_alloc_top.h"
#include "file.h"
#include "rbpaths.h"
#include "string-extra.h"
#include "system.h"
#include "usb.h"

/* KEYS_FILE: KEYS_MAGIC, the device count (uint32_t each, native order),
 * then that many struct bt_key as in memory */
#define KEYS_FILE   ROCKBOX_DIR "/bluetooth.dat"
#define KEYS_MAGIC  0x52424b31  /* "RBK1" */
/* CAL_FILE: the driver's radio calibration blob (bt_drv_cal_attach) */
#define CAL_FILE    ROCKBOX_DIR "/bluetooth.cal"
/* Driver open (a radio may calibrate first: YP3 ~5 s without a stored
 * calibration) plus HCI init */
#define POWER_ON_TIMEOUT (15 * HZ)
/* Power-off: how long to wait for the stream to close, then the link */
#define CLOSE_TIMEOUT       HZ
#define DISCONNECT_TIMEOUT  (2 * HZ)
#define SHUTDOWN_POLL       (HZ / 10)

#define RING_HDR    2           /* entry length */
#define RING_WRAP   0xffff      /* entry length: continue at offset 0 */
/* Ring space for a packet: length and packet, rounded up to 2 bytes */
#define RING_ENTRY(len) (((len) + RING_HDR + 1) & ~1u)

#define ADDR_STR_SIZE   18      /* "12:34:56:78:9A:BC" and NUL */

struct bt_state *bt;
static int bt_handle;
static volatile uint8_t cur_status = BT_STATUS_OFF;

/* RX ring: entries [length, 2 bytes][H4 packet], 2-byte aligned; a length
 * of RING_WRAP means "continue at 0". head == tail is empty, and the writer
 * never lets head catch up with tail. Written by the driver's receive
 * callback (any context), read by the thread. */
static bool ring_put(const uint8_t *p, size_t len)
{
    size_t need = RING_ENTRY(len);
    unsigned h = bt->rx_head, t = bt->rx_tail;

    if (h >= t) {
        size_t room = BT_RX_RING - h;
        if (need > room || (need == room && t == 0)) {
            if (need >= t)
                return false;
            put_le16(bt->rx_ring + h, RING_WRAP);
            h = 0;
        }
    } else if (need >= t - h) {
        return false;
    }
    put_le16(bt->rx_ring + h, len);
    memcpy(bt->rx_ring + h + RING_HDR, p, len);
    h += need;
    if (h == BT_RX_RING)
        h = 0;
    __asm__ volatile ("" ::: "memory");     /* entry written, then head */
    bt->rx_head = h;
    return true;
}

/* The driver's receive callback (any context): flow control at once,
 * events and ACL data to the ring for the thread. A packet bigger than
 * half the ring is dropped. */
static void bt_recv(const void *pkt, size_t len)
{
    const uint8_t *p = pkt;

    if (!bt || len < 3 || len > BT_RX_RING / 2)
        return;
    if (hci_rx_flow(p, len))
        return;
    if (p[0] != BT_H4_EVT && p[0] != BT_H4_ACL)
        return;
    if (!ring_put(p, len))
        logf("bt: rx ring full, %u bytes dropped", (unsigned)len);
    if (!bt->rx_posted) {
        bt->rx_posted = true;
        queue_post(&bt->queue, BT_EV_RX, 0);
    }
}

/* Thread: hand the queued packets to HCI */
static void rx_drain(void)
{
    bt->rx_posted = false;
    while (bt->rx_tail != bt->rx_head) {
        unsigned t = bt->rx_tail;
        size_t len = get_le16(bt->rx_ring + t);
        if (len == RING_WRAP) {
            bt->rx_tail = 0;
            continue;
        }
        const uint8_t *p = bt->rx_ring + t + RING_HDR;
        if (p[0] == BT_H4_EVT)
            hci_event(p + H4_HDR, len - H4_HDR);
        else
            hci_acl(p + H4_HDR, len - H4_HDR);
        t += RING_ENTRY(len);
        bt->rx_tail = t == BT_RX_RING ? 0 : t;
    }
}

/* Timers: deadlines in ticks, checked by the thread. 0 means off, so a
 * deadline that falls on tick 0 becomes tick 1. */
void bt_timer(int t, long ticks)
{
    long at = current_tick + ticks;
    bt->timers[t] = ticks ? (at ? at : 1) : 0;
}

/* Ticks until the next deadline, for the thread's queue wait */
static int next_timeout(void)
{
    long best = 0;
    for (int t = 0; t < T_COUNT; t++) {
        long at = bt->timers[t];
        if (at && (!best || TIME_BEFORE(at, best)))
            best = at;
    }
    if (!best)
        return TIMEOUT_BLOCK;
    long d = best - current_tick;
    return d < 1 ? 1 : d;
}

static void run_timers(void)
{
    for (int t = 0; t < T_COUNT; t++) {
        long at = bt->timers[t];
        if (!at || TIME_BEFORE(current_tick, at))
            continue;
        bt->timers[t] = 0;
        switch (t) {
        case T_SEC:
            hci_sec_timer();
            break;
        case T_AVDTP:
        case T_SUSPEND:
            avdtp_timer(t);
            break;
        case T_AVRCP:
            avrcp_timer();
            break;
        case T_HFP:
            hfp_timer();
            break;
        case T_PLAY:
            avrcp_play_timer();
            break;
        case T_REPAGE:
            hci_repage_timer();
            break;
        case T_ANNOUNCE:
            avdtp_announce_timer();
            break;
        case T_RESTART:
            hci_restart_timer();
            break;
#ifdef HAVE_BLUETOOTH_HID
        case T_HID:
            hid_timer();
            break;
#endif
        }
    }
}

/* Refresh the status that bt_status() returns without locking */
void bt_update_status(void)
{
    uint8_t s = BT_STATUS_OFF;
    if (bt && bt->up) {
        if (bt->av.state == ST_STREAMING)
            s = BT_STATUS_STREAMING;
        else if (bt->av.state != ST_IDLE)
            s = BT_STATUS_CONNECTED;
#ifdef HAVE_BLUETOOTH_HID
        else if (bt->hid.up)
            s = BT_STATUS_CONNECTED;
#endif
        else
            s = BT_STATUS_ON;
    }
    cur_status = s;
}

/* Paired devices */

/* Read KEYS_FILE; a missing or foreign file leaves the list empty */
static void keys_load(void)
{
    uint32_t hdr[2];
    int fd = open(KEYS_FILE, O_RDONLY);
    if (fd < 0)
        return;
    if (read(fd, hdr, sizeof hdr) == sizeof hdr && hdr[0] == KEYS_MAGIC &&
        hdr[1] <= BT_MAX_PAIRED) {
        size_t n = hdr[1] * sizeof bt->keys[0];
        if (read(fd, bt->keys, n) == (ssize_t)n)
            bt->nkeys = hdr[1];
    }
    close(fd);
}

/* Write the file if anything changed, but not while USB has the disk */
static void keys_save(void)
{
    if (bt->usb || !bt->keys_dirty)
        return;
    uint32_t hdr[2] = { KEYS_MAGIC, bt->nkeys };
    int fd = open(KEYS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return;
    write(fd, hdr, sizeof hdr);
    write(fd, bt->keys, bt->nkeys * sizeof bt->keys[0]);
    close(fd);
    bt->keys_dirty = false;
}

/* Radio calibration */

#ifdef HAVE_BLUETOOTH_DRV_CAL
/* Write a fresh calibration, but not while USB has the disk */
static void cal_save(void)
{
    if (bt->usb || !bt->cal_dirty)
        return;
    int fd = open(CAL_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return;
    bool ok = write(fd, bt->cal, bt->cal_len) == bt->cal_len;
    if (close(fd) == 0 && ok)
        bt->cal_dirty = false;
}

/* Read CAL_FILE into bt->cal and lend that to the driver for its open */
static void cal_load(void)
{
    int fd = open(CAL_FILE, O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, bt->cal, sizeof bt->cal);
        if (n > 0)
            bt->cal_len = n;
        close(fd);
    }
    bt_drv_cal_attach(bt->cal, sizeof bt->cal, bt->cal_len);
}

/* After a successful open: store what the driver measured, if anything */
static void cal_opened(void)
{
    size_t n = bt_drv_cal_fresh();
    if (n) {
        bt->cal_len = n;
        bt->cal_dirty = true;
        cal_save();
    }
}

/* After bt_drv_close() */
static void cal_release(void)
{
    bt_drv_cal_attach(NULL, 0, 0);
}
#else
static void cal_save(void) {}
static void cal_load(void) {}
static void cal_opened(void) {}
static void cal_release(void) {}
#endif

struct bt_key *bt_key_find(const bt_addr_t *addr)
{
    for (int i = 0; i < bt->nkeys; i++)
        if (!memcmp(&bt->keys[i].addr, addr, sizeof *addr))
            return &bt->keys[i];
    return NULL;
}

/* Move a paired device to the front (most recently used) */
static struct bt_key *key_to_front(struct bt_key *k)
{
    if (k != bt->keys) {
        struct bt_key tmp = *k;
        memmove(bt->keys + 1, bt->keys, (k - bt->keys) * sizeof *k);
        bt->keys[0] = tmp;
        bt->keys_dirty = true;
    }
    return bt->keys;
}

/* A link key from pairing: a new device takes the oldest one's place if
 * the list is full */
void bt_key_store(const bt_addr_t *addr, const uint8_t *key, uint8_t type)
{
    struct bt_key *k = bt_key_find(addr);
    if (!k) {
        if (bt->nkeys < BT_MAX_PAIRED)
            bt->nkeys++;
        k = &bt->keys[bt->nkeys - 1];   /* or replace the oldest */
        memset(k, 0, sizeof *k);
        k->addr = *addr;
        const char *name = bt_name_of(addr);
        if (!memcmp(&bt->link.addr, addr, sizeof *addr)) {
            name = bt->link.name;
            k->cod = bt->link.cod;
        }
        if (name)
            strlcpy(k->name, name, sizeof k->name);
        bt->keys_dirty = true;
    } else if (memcmp(k->key, key, LINK_KEY_LEN) || k->type != type) {
        bt->keys_dirty = true;
    }
    memcpy(k->key, key, LINK_KEY_LEN);
    k->type = type;
    key_to_front(k);
    keys_save();
}

/* The device's key worked: it becomes the most recently used */
void bt_key_used(const bt_addr_t *addr)
{
    struct bt_key *k = bt_key_find(addr);
    if (k) {
        key_to_front(k);
        keys_save();
    }
}

void bt_key_forget(const bt_addr_t *addr)
{
    struct bt_key *k = bt_key_find(addr);
    if (k) {
        memmove(k, k + 1, (bt->keys + --bt->nkeys - k) * sizeof *k);
        bt->keys_dirty = true;
        keys_save();
    }
}

/* A remote name arrived (len bytes, NUL-terminated or not): update the
 * link, the inquiry results and the paired list */
void bt_name_learned(const bt_addr_t *addr, const char *name, size_t len)
{
    char buf[BT_NAME_MAX];
    const char *nul = memchr(name, '\0', len);
    if (nul)
        len = nul - name;
    if (len >= sizeof buf)
        len = sizeof buf - 1;
    memcpy(buf, name, len);
    buf[len] = '\0';
    if (!memcmp(&bt->link.addr, addr, sizeof *addr))
        strcpy(bt->link.name, buf);
    for (int i = 0; i < bt->nfound; i++)
        if (!memcmp(&bt->found[i].dev.addr, addr, sizeof *addr))
            strcpy(bt->found[i].dev.name, buf);
    struct bt_key *k = bt_key_find(addr);
    if (k && strcmp(k->name, buf)) {
        strcpy(k->name, buf);
        bt->keys_dirty = true;
        keys_save();
    }
}

/* A device's name from the paired list or the inquiry results, or NULL */
const char *bt_name_of(const bt_addr_t *addr)
{
    struct bt_key *k = bt_key_find(addr);
    if (k && k->name[0])
        return k->name;
    for (int i = 0; i < bt->nfound; i++)
        if (!memcmp(&bt->found[i].dev.addr, addr, sizeof *addr) &&
            bt->found[i].dev.name[0])
            return bt->found[i].dev.name;
    return NULL;
}

/* Thread */

/* Controller set-up finished (hci.c): wake bt_power() */
void bt_init_done(bool ok)
{
    bt->up = ok;
    if (ok)
        bt_update_status();
    semaphore_release(&bt->ready);
}

/* The controller kept a link that the stack let go (hci.c, T_RESTART):
 * close and open the driver, which drops it, and set the controller up
 * again; the stack's own state carries on */
void bt_restart(void)
{
    bt->up = false;
    bt_drv_close();
    if (bt_drv_open(bt_recv)) {
        cal_opened();
        hci_start();
    } else {
        bt_init_done(false);
    }
}

/* USB takes the disk: no key writes until it gives it back */
static bool handle_usb(const struct queue_event *ev)
{
    if (ev->id == SYS_USB_CONNECTED) {
        bt->usb = true;
        usb_acknowledge(SYS_USB_CONNECTED_ACK, ev->data);
    } else if (ev->id == SYS_USB_DISCONNECTED) {
        bt->usb = false;
        keys_save();
        cal_save();
    } else {
        return false;
    }
    return true;
}

static bool stream_open(void)
{
    return bt->av.state != ST_IDLE;
}

static bool link_up(void)
{
    return bt->link.state != LINK_NONE;
}

/* Power-off: handle received packets (and USB) while busy(), at most for
 * the given ticks */
static void wait_while(bool (*busy)(void), long ticks)
{
    struct queue_event ev;
    long end = current_tick + ticks;
    while (busy() && TIME_BEFORE(current_tick, end)) {
        queue_wait_w_tmo(&bt->queue, &ev, SHUTDOWN_POLL);
        handle_usb(&ev);
        rx_drain();
    }
}

/* Close the stream and the link so the headset knows, then power down */
static void shutdown(void)
{
    if (bt->up) {
        if (avdtp_close())
            wait_while(stream_open, CLOSE_TIMEOUT);
        hci_power_off();
        wait_while(link_up, DISCONNECT_TIMEOUT);
        l2cap_link_down();
    }
    bt->up = false;
    bt_update_status();
    bt_drv_close();
    cal_release();
    keys_save();
    cal_save();
}

static void bt_thread(void)
{
    struct queue_event ev;

    keys_load();
    cal_load();
    if (bt_drv_open(bt_recv)) {
        cal_opened();
        hci_start();
    } else {
        bt_init_done(false);
    }

    for (;;) {
        queue_wait_w_tmo(&bt->queue, &ev, next_timeout());
        if (ev.id == BT_MSG_OFF)
            break;
        if (!handle_usb(&ev) && bt->up) {
            switch (ev.id) {
            case BT_MSG_SCAN:
                hci_scan(ev.data);
                break;
            case BT_MSG_CONNECT:
                hci_connect(&bt->connect_addr);
                break;
            case BT_MSG_DISCONNECT:
                bt->connect_pending = false;
                hci_disconnect();
                break;
            case BT_MSG_FORGET:
                if (!memcmp(&bt->link.addr, &bt->forget_addr, sizeof(bt_addr_t)))
                    hci_disconnect();
                if (!memcmp(&bt->lost_addr, &bt->forget_addr, sizeof(bt_addr_t)))
                    bt->repages = REPAGE_STOP;  /* page it no more */
                bt_key_forget(&bt->forget_addr);
                break;
            case BT_MSG_STREAM:
                avdtp_stream_want(ev.data);
                break;
            case BT_MSG_VOLUME:
                avrcp_volume();
                break;
            }
        }
        rx_drain();
        if (bt->up)
            run_timers();
        bt_update_status();
    }
    shutdown();
    thread_exit();
}

/* Caller's thread: stop the Bluetooth thread and free the state */
static void power_off(void)
{
    if (!bt)
        return;
    queue_post(&bt->queue, BT_MSG_OFF, 0);
    thread_wait(bt->thread);
    queue_delete(&bt->queue);
    bt = NULL;
    bt_sbc_release();
    core_free(bt_handle);
    cur_status = BT_STATUS_OFF;
}

/* Public API */
enum bt_status bt_status(void)
{
    return cur_status;
}

/* Power on: allocate the state and the encoder's memory, start the thread
 * and wait for the controller (a stack that is not up is stopped first);
 * or power off */
bool bt_power(bool on)
{
    if (!on || (bt && !bt->up))
        power_off();
    if (!on || bt)
        return true;

    int h = core_alloc_top(sizeof *bt);     /* pinned while on */
    if (h <= 0)
        return false;
    if (!bt_sbc_reserve()) {
        core_free(h);
        return false;
    }
    core_pin(h);
    struct bt_state *s = core_get_data(h);
    memset(s, 0, sizeof *s);
    queue_init(&s->queue, true);
    semaphore_init(&s->ready, 1, 0);
    semaphore_init(&s->credit, 1, 0);
    bt_handle = h;
    bt = s;
    s->thread = create_thread(bt_thread, s->stack, sizeof s->stack, 0,
                              "bluetooth" IF_PRIO(, PRIORITY_SYSTEM)
                              IF_COP(, CPU));
    if (!s->thread) {
        queue_delete(&s->queue);
        bt = NULL;
        bt_sbc_release();
        core_free(h);
        return false;
    }
    semaphore_wait(&s->ready, POWER_ON_TIMEOUT);
    if (!s->up) {
        power_off();
        return false;
    }
    return true;
}

bool bt_scan(bool start)
{
    if (!bt || !bt->up)
        return false;
    if (start)
        bt->scan = SCAN_INQUIRY;    /* bt_scanning() from now on */
    queue_post(&bt->queue, BT_MSG_SCAN, start);
    return true;
}

bool bt_scanning(void)
{
    return bt && bt->scan != SCAN_IDLE;
}

int bt_scan_results(struct bt_device *devs, int max)
{
    int n = 0;
    for (; bt && n < bt->nfound && n < max; n++)
        devs[n] = bt->found[n].dev;
    return n;
}

bool bt_connect(const bt_addr_t *addr)
{
    if (!bt || !bt->up)
        return false;
    bt->connect_addr = *addr;
    queue_post(&bt->queue, BT_MSG_CONNECT, 0);
    return true;
}

void bt_disconnect(void)
{
    if (bt)
        queue_post(&bt->queue, BT_MSG_DISCONNECT, 0);
}

bool bt_connected_device(struct bt_device *dev)
{
    if (!bt || bt->link.state == LINK_NONE)
        return false;
    dev->addr = bt->link.addr;
    dev->cod = bt->link.cod;
    strcpy(dev->name, bt->link.name);
    return true;
}

bool bt_pairing_code(char *buf)
{
    if (!bt || !bt->code[0])
        return false;
    strlcpy(buf, bt->code, BT_CODE_MAX);
    return true;
}

int bt_paired_devices(struct bt_device *devs, int max)
{
    int n = 0;
    for (; bt && n < bt->nkeys && n < max; n++) {
        devs[n].addr = bt->keys[n].addr;
        devs[n].cod = bt->keys[n].cod;
        strcpy(devs[n].name, bt->keys[n].name);
    }
    return n;
}

bool bt_forget(const bt_addr_t *addr)
{
    if (!bt || !bt_key_find(addr))
        return false;
    bt->forget_addr = *addr;
    queue_post(&bt->queue, BT_MSG_FORGET, 0);
    return true;
}

void bt_addr_format(const bt_addr_t *a, char *buf)
{
    snprintf(buf, ADDR_STR_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X",
             a->b[5], a->b[4], a->b[3], a->b[2], a->b[1], a->b[0]);
}

/* Most significant byte first, two hex digits each, ':' between */
bool bt_addr_parse(const char *s, bt_addr_t *a)
{
    for (int i = 5; i >= 0; i--) {
        int v = 0;
        for (int j = 0; j < 2; j++, s++) {
            int c = *s | 0x20;      /* ASCII lower case */
            if (*s >= '0' && *s <= '9')
                v = v * 16 + *s - '0';
            else if (c >= 'a' && c <= 'f')
                v = v * 16 + c - 'a' + 10;
            else
                return false;
        }
        a->b[i] = v;
        if (*s != (i ? ':' : '\0'))
            return false;
        s++;
    }
    return true;
}
