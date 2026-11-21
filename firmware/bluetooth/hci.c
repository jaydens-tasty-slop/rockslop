/* SPDX-License-Identifier: GPL-2.0-or-later */
/* HCI (Core spec Vol 4, Part E): controller set-up, command and ACL flow
 * control, the ACL link, security (Secure Simple Pairing "Just Works",
 * legacy PIN 0000; a keyboard types a passkey we show), inquiry and
 * remote names. A link lost while it
 * carried a stream is paged again for a while (T_REPAGE); one that the
 * controller keeps after we let it go restarts the controller (T_RESTART).
 *
 * Commands go out one at a time per command credit; events come from the
 * RX ring via hci_event() in the Bluetooth thread, except that credits
 * are taken straight from the driver callback (hci_rx_flow) so that the
 * media tick sees ACL credits return without waiting for the thread. */
#include "bt.h"
#include <stdio.h>
#include <string.h>
#include "string-extra.h"
#include "system.h"

/* Command opcodes (OGF << 10 | OCF). Link Control: */
#define OP_INQUIRY              0x0401
#define OP_INQUIRY_CANCEL       0x0402
#define OP_CREATE_CONN          0x0405
#define OP_DISCONNECT           0x0406
#define OP_CREATE_CONN_CANCEL   0x0408
#define OP_ACCEPT_CONN          0x0409
#define OP_REJECT_CONN          0x040a
#define OP_LINK_KEY_REPLY       0x040b
#define OP_LINK_KEY_NEG_REPLY   0x040c
#define OP_PIN_REPLY            0x040d
#define OP_AUTH_REQUESTED       0x0411
#define OP_SET_ENCRYPTION       0x0413
#define OP_REMOTE_NAME          0x0419
#define OP_IO_CAP_REPLY         0x042b
#define OP_USER_CONFIRM_REPLY   0x042c
#define OP_PASSKEY_NEG_REPLY    0x042f
/* Link Policy */
#define OP_WRITE_LINK_POLICY    0x080f  /* Write_Default_Link_Policy_Settings */
/* Controller & Baseband */
#define OP_SET_EVENT_MASK       0x0c01
#define OP_RESET                0x0c03
#define OP_WRITE_LOCAL_NAME     0x0c13
#define OP_WRITE_PAGE_TIMEOUT   0x0c18
#define OP_WRITE_SCAN_ENABLE    0x0c1a
#define OP_WRITE_COD            0x0c24
#define OP_WRITE_SUPERVISION    0x0c37  /* Write_Link_Supervision_Timeout */
#define OP_WRITE_INQUIRY_MODE   0x0c45
#define OP_WRITE_SSP_MODE       0x0c56
/* Informational */
#define OP_READ_BUFFER_SIZE     0x1005
#define OP_READ_BD_ADDR         0x1009

/* Event codes */
#define EV_INQUIRY_COMPLETE     0x01
#define EV_INQUIRY_RESULT       0x02
#define EV_CONN_COMPLETE        0x03
#define EV_CONN_REQUEST         0x04
#define EV_DISCONN_COMPLETE     0x05
#define EV_AUTH_COMPLETE        0x06
#define EV_REMOTE_NAME          0x07
#define EV_ENCRYPT_CHANGE       0x08
#define EV_CMD_COMPLETE         0x0e
#define EV_CMD_STATUS           0x0f
#define EV_HW_ERROR             0x10
#define EV_ROLE_CHANGE          0x12
#define EV_NUM_COMPLETED        0x13
#define EV_PIN_REQUEST          0x16
#define EV_LINK_KEY_REQUEST     0x17
#define EV_LINK_KEY_NOTIFY      0x18
#define EV_INQUIRY_RESULT_RSSI  0x22
#define EV_INQUIRY_RESULT_EXT   0x2f
#define EV_IO_CAP_REQUEST       0x31
#define EV_USER_CONFIRM         0x33
#define EV_USER_PASSKEY         0x34
#define EV_SSP_COMPLETE         0x36
#define EV_PASSKEY_NOTIFY       0x3b

/* Error codes */
#define ERR_KEY_MISSING         0x06
#define ERR_CONN_EXISTS         0x0b    /* ACL connection already exists */
#define ERR_DISALLOWED          0x0c
#define ERR_LIMITED_RESOURCES   0x0d
#define ERR_USER_TERMINATED     0x13
#define ERR_POWER_OFF           0x15    /* remote: terminated, powering off */
#define ERR_LMP_COLLISION       0x23
#define ERR_DIFF_COLLISION      0x2a

/* Packet layout */
#define CMD_PKT_HDR     (H4_HDR + 3)    /* + opcode, parameter length */
#define ACL_PKT_HDR     (H4_HDR + ACL_HDR)  /* in front of each fragment */
#define HANDLE_MASK     0x0fff          /* connection handle: 12 bits */
/* ACL packet boundary flag, in the high byte of the handle field */
#define ACL_PB_MASK     0x30
#define ACL_PB_CONT     0x10            /* continuing fragment */
#define ACL_PB_FIRST    0x20            /* first, automatically flushable */
#define BD_ADDR_LEN     6
#define NAME_LEN        248             /* local and remote names */
#define PIN_LEN         16              /* PIN_Code parameter */
#define EIR_NAME_SHORT  0x08            /* EIR data types */
#define EIR_NAME_FULL   0x09

/* Parameter values */
#define SCAN_ENABLE_NONE        0x00
#define SCAN_ENABLE_PAGE        0x02    /* page scan only: not discoverable */
#define SSP_MODE_ON             0x01
#define INQUIRY_MODE_EIR        0x02    /* EIR results: names without paging */
#define PAGE_TIMEOUT            0x2000  /* x 0.625 ms: 5.12 s */
#define SUPERVISION_TIMEOUT     0x1f40  /* x 0.625 ms: 5 s (stock's; default 20 s) */
#define LINK_POLICY             0x0005  /* role switch, sniff mode */
#define ACL_PACKET_TYPES        0xcc18  /* DM1 DH1 DM3 DH3 DM5 DH5 */
#define PSRM_MASK               0x03    /* page scan repetition mode */
#define PSRM_DEFAULT            0x01    /* R1, when inquiry did not say */
#define PSRM_NAME_ASKED         0x80    /* bt_found.psrm: name requested */
#define CLOCK_OFFSET_VALID      0x8000
#define ERR_CONN_TIMEOUT        0x08    /* supervision timeout: out of range */
#define ERR_LMP_TIMEOUT         0x22    /* LMP response timeout */
#define ALLOW_ROLE_SWITCH       0x01
#define LINK_TYPE_ACL           0x01
#define ROLE_CENTRAL            0x00
#define ROLE_PERIPHERAL         0x01    /* accept: stay peripheral */
#define ENCRYPTION_ON           0x01
#define IO_CAP_DISPLAY          0x00    /* DisplayOnly */
#define IO_CAP_NO_IO            0x03    /* NoInputNoOutput */
#define OOB_NONE                0x00
#define AUTH_GENERAL_BONDING    0x04    /* no MITM protection: Just Works */
#define AUTH_MITM_BONDING       0x05    /* with MITM: a keyboard's passkey */
#define LEGACY_PIN              "0000"
#define GIAC_LAP                0x9e8b33    /* General Inquiry Access Code */
#define INQUIRY_LENGTH          8       /* x 1.28 s: 10.24 s */
#define INQUIRY_RESP_LEN        14      /* per response, array formats */

/* Timing */
#define CMD_CREDIT_WAIT (2 * HZ)        /* then send without a credit */
#define ACL_TRIES       10              /* thread: wait for an ACL credit */
#define ACL_CREDIT_WAIT (HZ / 5)        /* ...this long each time */
#define CMD_SEND_TRIES  10              /* a driver out of room, a tick apart */
#define SEC_TIMEOUT     (30 * HZ)       /* authentication + encryption */
#define SEC_RETRY       (HZ / 2)        /* after a collision */
#define REPAGE_DELAY    (5 * HZ)        /* audio link lost: page it again */
#define REPAGE_PERIOD   (15 * HZ)       /* ...this often */
#define PAGE_RETRIES    3               /* a failed page: tries again */
#define CLOSE_WAIT      (30 * HZ)       /* a Disconnect completes by then:
                                         * supervision timeouts are 5-20 s */

#ifdef BT_LOCAL_NAME
#define LOCAL_NAME      BT_LOCAL_NAME   /* the target's controller keeps less */
#else
#define LOCAL_NAME      "Rockbox " MODEL_NAME
#endif
#define LOCAL_COD       0x20041c    /* audio service; A/V, portable audio */

/* Driver context: credits come back without waiting for the thread.
 * p is the H4 packet: [BT_H4_EVT][code][parameter length][parameters].
 * Returns true if the packet is used up (Number Of Completed Packets). */
bool hci_rx_flow(const uint8_t *p, size_t len)
{
    if (p[0] != BT_H4_EVT)
        return false;
    switch (p[1]) {
    case EV_NUM_COMPLETED: {
        /* Num_Handles, then {handle, completed} pairs: there is only one
         * link, so every handle counts */
        unsigned n = p[3], add = 0;
        for (unsigned i = 0; i < n && 8 + 4 * i <= len; i++)
            add += get_le16(p + 6 + 4 * i);
        int irq = disable_irq_save();
        unsigned c = bt->acl_credits + add;
        bt->acl_credits = MIN(c, bt->acl_total);
        restore_irq(irq);
        semaphore_release(&bt->credit);
        return true;
    }
    case EV_CMD_COMPLETE:           /* Num_HCI_Command_Packets first */
        if (len >= 4) {
            bt->cmd_credits = p[3];
            semaphore_release(&bt->credit);
        }
        break;
    case EV_CMD_STATUS:             /* status, then Num_HCI_Command_Packets */
        if (len >= 5) {
            bt->cmd_credits = p[4];
            semaphore_release(&bt->credit);
        }
        break;
    }
    return false;
}

/* The thread sends with tx_busy set; the media tick skips its turn
 * meanwhile, so the driver never sees nested sends and ACL fragments of
 * different PDUs never interleave. Interrupts are off for the claim so
 * that in the simulator, where the tick runs in parallel, it is not
 * halfway through a media packet. */
static void tx_claim(void)
{
    int irq = disable_irq_save();
    bt->tx_busy = true;
    restore_irq(irq);
}

/* Send a command: [BT_H4_CMD][opcode][length][parameters]. Waits for a
 * command credit, but not forever: a controller that lost one would
 * otherwise stall the stack. */
static void hci_cmd(uint16_t op, const void *param, size_t len)
{
    uint8_t pkt[CMD_PKT_HDR + NAME_LEN];    /* the longest: Write_Local_Name */
    pkt[0] = BT_H4_CMD;
    put_le16(pkt + 1, op);
    pkt[3] = len;
    memcpy(pkt + CMD_PKT_HDR, param, len);
    if (!bt->up)
        bt->init_op = op;
    while (!bt->cmd_credits) {
        if (semaphore_wait(&bt->credit, CMD_CREDIT_WAIT) == OBJ_WAIT_TIMEDOUT &&
            !bt->cmd_credits) {
            logf("bt: no command credit, sending %04x anyway", op);
            break;
        }
    }
    int irq = disable_irq_save();
    bool credit = bt->cmd_credits;
    if (credit)
        bt->cmd_credits--;
    restore_irq(irq);
    for (int tries = 1; ; tries++) {
        tx_claim();
        bool sent = bt_drv_send(pkt, len + CMD_PKT_HDR);
        bt->tx_busy = false;
        if (sent)
            return;
        if (tries == CMD_SEND_TRIES)
            break;
        sleep(1);
    }
    logf("bt: driver refused command %04x", op);
    if (credit) {
        irq = disable_irq_save();
        bt->cmd_credits++;
        restore_irq(irq);
    }
}

/* A command whose only parameter is a BD_ADDR */
static void cmd_addr(uint16_t op, const bt_addr_t *a)
{
    hci_cmd(op, a, BD_ADDR_LEN);
}

/* A command on the link: the handle (len 2), or the handle and one byte
 * (len 3) */
static void cmd_handle(uint16_t op, uint8_t extra, size_t len)
{
    uint8_t p[3];
    put_le16(p, bt->link.handle);
    p[2] = extra;
    hci_cmd(op, p, len);
}

/* ACL: one L2CAP PDU at buf + BT_HDR - 4 (payload len at buf + BT_HDR),
 * sent as fragments of at most acl_mtu with the headers written in front
 * of each. Needs a credit per fragment, plus keep credits left over. A
 * later fragment's headers borrow the end of the previous one, which is
 * put back after sending. If the driver refuses a fragment, the credits
 * not used go back and the caller may send the whole PDU again (the peer
 * drops a PDU cut short once the next one starts). */
static bool acl_out(uint8_t *buf, uint16_t cid, size_t len, unsigned keep)
{
    size_t pdu = len + L2CAP_HDR;
    unsigned frags = (pdu + bt->acl_mtu - 1) / bt->acl_mtu;
    bool ok = false;

    int irq = disable_irq_save();
    if (bt->acl_credits >= frags + keep) {
        bt->acl_credits -= frags;
        ok = true;
    }
    restore_irq(irq);
    if (!ok)
        return false;

    put_le16(buf + ACL_PKT_HDR, len);       /* L2CAP header */
    put_le16(buf + ACL_PKT_HDR + 2, cid);
    uint8_t *p = buf + ACL_PKT_HDR;
    for (uint8_t pb = ACL_PB_FIRST; pdu; pb = ACL_PB_CONT) {
        size_t n = MIN(pdu, bt->acl_mtu);
        uint8_t *h = p - ACL_PKT_HDR, save[ACL_PKT_HDR];
        memcpy(save, h, ACL_PKT_HDR);
        h[0] = BT_H4_ACL;
        h[1] = bt->link.handle;
        h[2] = (bt->link.handle >> 8) | pb;
        put_le16(h + 3, n);
        bool sent = bt_drv_send(h, n + ACL_PKT_HDR);
        if (pb == ACL_PB_CONT)
            memcpy(h, save, ACL_PKT_HDR);
        if (!sent) {
            irq = disable_irq_save();
            unsigned c = bt->acl_credits + (pdu + bt->acl_mtu - 1) / bt->acl_mtu;
            bt->acl_credits = MIN(c, bt->acl_total);
            restore_irq(irq);
            return false;
        }
        p += n;
        pdu -= n;
    }
    return true;
}

/* The media path (tick task) sends only if credits are free now, leaving
 * one for signalling when the controller has more than two; the thread
 * waits a while for credits (or for room in the driver). */
bool acl_send(uint8_t *buf, uint16_t cid, size_t len, bool media)
{
    if (bt->link.state != LINK_UP)
        return false;
    if (media)
        return acl_out(buf, cid, len, bt->acl_total > 2);

    for (int tries = 0; tries < ACL_TRIES; tries++) {
        tx_claim();
        bool ok = acl_out(buf, cid, len, 0);
        bt->tx_busy = false;
        if (ok)
            return true;
        semaphore_wait(&bt->credit, ACL_CREDIT_WAIT);
        if (bt->link.state != LINK_UP)
            break;
    }
    logf("bt: no ACL credit, PDU dropped");
    return false;
}

/* Inbound ACL (p after the H4 indicator): reassemble L2CAP PDUs. A start
 * fragment carries the L2CAP length, which says how much is still to
 * come. */
void hci_acl(const uint8_t *p, size_t len)
{
    if (len < ACL_HDR || (get_le16(p) & HANDLE_MASK) != bt->link.handle)
        return;
    size_t n = get_le16(p + 2);
    uint8_t pb = p[1] & ACL_PB_MASK;
    p += ACL_HDR;
    if (n > len - ACL_HDR)
        return;
    if (pb != ACL_PB_CONT) {
        bt->rx_pdu_len = 0;
        if (n < L2CAP_HDR)
            return;
        size_t want = get_le16(p) + L2CAP_HDR;
        if (n == want) {
            l2cap_rx(p, n);
            return;
        }
        if (n > want || want > sizeof bt->rx_pdu) {
            logf("bt: oversized PDU dropped");
            return;
        }
        bt->rx_pdu_want = want;
    } else if (!bt->rx_pdu_len || bt->rx_pdu_len + n > bt->rx_pdu_want) {
        bt->rx_pdu_len = 0;
        return;
    }
    memcpy(bt->rx_pdu + bt->rx_pdu_len, p, n);
    bt->rx_pdu_len += n;
    if (bt->rx_pdu_len == bt->rx_pdu_want) {
        bt->rx_pdu_len = 0;
        l2cap_rx(bt->rx_pdu, bt->rx_pdu_want);
    }
}

/* Page scan takes air time from the link (on the YP3, 140 ms audio gaps
 * every 0.6 s), and one link is all we take: connectable only without one */
static void set_connectable(bool on)
{
    uint8_t p = on ? SCAN_ENABLE_PAGE : SCAN_ENABLE_NONE;
    hci_cmd(OP_WRITE_SCAN_ENABLE, &p, 1);
}

/* Controller set-up, one command per Command Complete (init_complete) */
static void init_next(void)
{
    /* all events on; bits 18, 35-42, 54, 57 and 62-63 (reserved) off */
    static const uint8_t event_mask[8] = {
        0xff, 0xff, 0xfb, 0xff, 0x07, 0xf8, 0xbf, 0x3d
    };
    uint8_t p[NAME_LEN];

    memset(p, 0, sizeof p);
    bt->init_op = 0;
    switch (bt->init_step++) {
    case 0:
        hci_cmd(OP_RESET, NULL, 0);
        break;
    case 1:
        hci_cmd(OP_READ_BUFFER_SIZE, NULL, 0);
        break;
    case 2:
        hci_cmd(OP_READ_BD_ADDR, NULL, 0);
        break;
    case 3:
        hci_cmd(OP_SET_EVENT_MASK, event_mask, sizeof event_mask);
        break;
    case 4:
        strlcpy((char *)p, LOCAL_NAME, sizeof p);
        hci_cmd(OP_WRITE_LOCAL_NAME, p, NAME_LEN);
        break;
    case 5:
        p[0] = LOCAL_COD & 0xff;
        p[1] = (LOCAL_COD >> 8) & 0xff;
        p[2] = LOCAL_COD >> 16;
        hci_cmd(OP_WRITE_COD, p, 3);
        break;
    case 6:
        p[0] = SSP_MODE_ON;
        hci_cmd(OP_WRITE_SSP_MODE, p, 1);
        break;
    case 7:
        p[0] = INQUIRY_MODE_EIR;
        hci_cmd(OP_WRITE_INQUIRY_MODE, p, 1);
        break;
    case 8:
        put_le16(p, PAGE_TIMEOUT);
        hci_cmd(OP_WRITE_PAGE_TIMEOUT, p, 2);
        break;
    case 9:
        put_le16(p, LINK_POLICY);
        hci_cmd(OP_WRITE_LINK_POLICY, p, 2);
        break;
    case 10:
        set_connectable(true);  /* not discoverable */
        break;
    default:
        logf("bt: controller up, ACL %u x %u", bt->acl_total, bt->acl_mtu);
        bt_init_done(true);
        if (bt->connect_pending) {  /* waited for a link the restart dropped */
            bt->connect_pending = false;
            hci_connect(&bt->connect_addr);
        }
        break;
    }
}

/* At power-on, and again after a restart (T_RESTART) */
void hci_start(void)
{
    bt->cmd_credits = 1;
    bt->init_step = 0;
    bt->scan = SCAN_IDLE;
    bt->link.state = LINK_NONE;
    init_next();
}

/* Command Complete for a set-up command: r points at the return
 * parameters after the status. Reset and the reads must work; the rest
 * may be unsupported. */
static void init_complete(uint16_t op, uint8_t status, const uint8_t *r, size_t len)
{
    if (status && (op == OP_RESET || op == OP_READ_BUFFER_SIZE ||
                   op == OP_READ_BD_ADDR)) {
        logf("bt: init %04x failed: %02x", op, status);
        bt_init_done(false);
        return;
    }
    if (op == OP_READ_BUFFER_SIZE && len >= 5) {
        /* ACL length (2), SCO length (1), ACL packets (2), SCO packets */
        bt->acl_mtu = MIN(get_le16(r), BT_MEDIA_SIZE);
        bt->acl_total = bt->acl_credits = get_le16(r + 3);
        if (!bt->acl_mtu || !bt->acl_total) {
            bt_init_done(false);
            return;
        }
    } else if (op == OP_READ_BD_ADDR && len >= BD_ADDR_LEN) {
        memcpy(&bt->local, r, BD_ADDR_LEN);
    } else if (status) {
        logf("bt: init %04x unsupported: %02x", op, status);
    }
    init_next();
}

/* The link */
static struct bt_found *found_find(const uint8_t *addr)
{
    for (int i = 0; i < bt->nfound; i++)
        if (!memcmp(&bt->found[i].dev.addr, addr, BD_ADDR_LEN))
            return &bt->found[i];
    return NULL;
}

/* A new link to addr is being set up: take its name and class from the
 * paired list or the inquiry results */
static void link_init(const uint8_t *addr, uint8_t flags)
{
    struct bt_link *l = &bt->link;
    memset(l, 0, sizeof *l);
    memcpy(&l->addr, addr, BD_ADDR_LEN);
    l->state = LINK_PAGING;
    l->flags = flags;
    const char *name = bt_name_of(&l->addr);
    if (name)
        strlcpy(l->name, name, sizeof l->name);
    struct bt_found *f = found_find(addr);
    struct bt_key *k = bt_key_find(&l->addr);
    l->cod = f ? f->dev.cod : k ? k->cod : 0;
    if (k)
        l->flags |= LF_BONDED;
}

/* Forget the link: its channels, credits and timers */
static void link_forget(void)
{
    l2cap_link_down();
    int irq = disable_irq_save();
    bt->acl_credits = bt->acl_total;
    restore_irq(irq);
    bt->rx_pdu_len = 0;
    bt->link.state = LINK_NONE;
    bt->code[0] = '\0';
    bt_timer(T_SEC, 0);
    bt_timer(T_RESTART, 0);
}

/* The link is gone (or never came up): forget it, become connectable
 * again and make a connection that waited for this */
static void link_down(void)
{
    set_connectable(true);
    link_forget();
    if (bt->connect_pending) {
        bt->connect_pending = false;
        hci_connect(&bt->connect_addr);
    }
}

/* The link was lost with a stream (out of range, or silent: dropped by
 * us): a WH-1000XM3 does not page us again, so we page it; while paging,
 * its next link lost before a stream (still at the edge) as well */
static void link_lost(void)
{
    struct bt_link *l = &bt->link;
    if (bt->av.announced) {
        bt->lost_addr = l->addr;
        bt->repages = 0;
        bt_timer(T_REPAGE, REPAGE_DELAY);
    } else if (bt->repages && !memcmp(&l->addr, &bt->lost_addr, BD_ADDR_LEN)) {
        bt_timer(T_REPAGE, REPAGE_DELAY);
    }
}

/* A page failed; one to a device that the controller still has a link to
 * (ours forgotten) restarts it */
static void page_failed(uint8_t status)
{
    struct bt_link *l = &bt->link;
    /* a device that did not answer (auto-connect at power-on, the menu)
     * is paged a few times more, through the re-page timer, unless that
     * is already paging it (YP3 gate torture bt-onoff 727: one page
     * timeout and no link for 25 s) */
    bool retry = (l->flags & LF_OUTGOING) && status != ERR_CONN_EXISTS &&
                 !bt->repages;
    bt_addr_t addr = l->addr;
    link_down();
    if (status == ERR_CONN_EXISTS)
        bt_timer(T_RESTART, 1);
    else if (retry) {
        bt->lost_addr = addr;
        bt->repages = REPAGE_STOP - PAGE_RETRIES;
        bt_timer(T_REPAGE, REPAGE_DELAY);
    }
}

/* Page a device; a link to another device is closed first */
void hci_connect(const bt_addr_t *a)
{
    struct bt_link *l = &bt->link;
    if (l->state != LINK_NONE) {
        if (memcmp(&l->addr, a, BD_ADDR_LEN)) {
            bt->connect_addr = *a;
            bt->connect_pending = true;
            hci_disconnect();
        }
        return;
    }
    if (bt->scan == SCAN_INQUIRY)
        hci_cmd(OP_INQUIRY_CANCEL, NULL, 0);
    bt->scan = SCAN_IDLE;

    /* BD_ADDR, packet types (2), PSRM, reserved, clock offset (2),
     * allow role switch */
    uint8_t p[13];
    struct bt_found *f = found_find(a->b);
    link_init(a->b, LF_OUTGOING);
    memcpy(p, a, BD_ADDR_LEN);
    put_le16(p + 6, ACL_PACKET_TYPES);
    p[8] = f ? f->psrm & PSRM_MASK : PSRM_DEFAULT;
    p[9] = 0;
    put_le16(p + 10, f ? f->clock : 0);
    p[12] = ALLOW_ROLE_SWITCH;
    hci_cmd(OP_CREATE_CONN, p, sizeof p);
}

/* As central, drop a link that stopped answering after 5 s, as stock
 * does, not after the default 20 s (the stream stands still meanwhile).
 * Only the central's setting counts. */
static void set_supervision_timeout(void)
{
    struct bt_link *l = &bt->link;
    if (l->state != LINK_UP || (l->flags & LF_PERIPHERAL))
        return;
    uint8_t p[4];                   /* handle, timeout */
    put_le16(p, l->handle);
    put_le16(p + 2, SUPERVISION_TIMEOUT);
    hci_cmd(OP_WRITE_SUPERVISION, p, sizeof p);
}

/* Close the link with the given reason, or cancel our page */
static void disconnect(uint8_t reason)
{
    struct bt_link *l = &bt->link;
    if (l->state == LINK_UP) {
        cmd_handle(OP_DISCONNECT, reason, 3);
        l->state = LINK_CLOSING;
        bt_timer(T_RESTART, CLOSE_WAIT);
    } else if (l->state == LINK_PAGING && (l->flags & LF_OUTGOING)) {
        cmd_addr(OP_CREATE_CONN_CANCEL, &l->addr);
    }
}

void hci_disconnect(void)
{
    disconnect(ERR_USER_TERMINATED);
}

/* A link whose remote stopped answering is lost as by a supervision
 * timeout, which may come later (the headset's, as central: 20 s) */
void hci_drop_silent(void)
{
    bt->link.flags |= LF_SILENT;
    hci_disconnect();
}

void hci_power_off(void)
{
    disconnect(ERR_POWER_OFF);
}

/* Remote_Name_Request: BD_ADDR, PSRM, reserved, clock offset */
static void name_request(const bt_addr_t *a, uint8_t psrm, uint16_t clock)
{
    uint8_t p[10];
    memcpy(p, a, BD_ADDR_LEN);
    p[6] = psrm;
    p[7] = 0;
    put_le16(p + 8, clock);
    hci_cmd(OP_REMOTE_NAME, p, sizeof p);
}

/* Security: authenticate, then encrypt; L2CAP waits for the result */
void hci_secure(void)
{
    struct bt_link *l = &bt->link;
    if (l->state != LINK_UP || (l->flags & LF_SEC_BUSY))
        return;
    if (l->flags & LF_ENCRYPT) {
        l2cap_link_secured(true);
        return;
    }
    l->flags |= LF_SEC_BUSY;
    bt_timer(T_SEC, SEC_TIMEOUT);
    if (!(l->flags & LF_AUTH))
        cmd_handle(OP_AUTH_REQUESTED, 0, 2);
    else
        cmd_handle(OP_SET_ENCRYPTION, ENCRYPTION_ON, 3);
}

/* Authentication or encryption failed: pair afresh, retry after a
 * collision, or give up (L2CAP then drops the link) */
static void sec_failed(uint8_t status)
{
    struct bt_link *l = &bt->link;
    l->flags &= ~LF_SEC_BUSY;
    if (status == ERR_KEY_MISSING && !(l->flags & LF_KEY_RETRY) &&
        bt_key_find(&l->addr)) {
        /* the headset forgot us: pair afresh */
        logf("bt: stored key rejected, pairing again");
        l->flags |= LF_KEY_RETRY;
        bt_key_forget(&l->addr);
        hci_secure();
    } else if (status == ERR_LMP_COLLISION || status == ERR_DIFF_COLLISION ||
               status == ERR_DISALLOWED) {
        bt_timer(T_SEC, SEC_RETRY);     /* the peer is busy securing the link */
    } else {
        logf("bt: security failed: %02x", status);
        l2cap_link_secured(false);
    }
}

/* T_REPAGE: page the device whose link was lost while there is no link
 * or inquiry; any link that comes up ends it (EV_CONN_COMPLETE), even one
 * closed again before the next try, and EV_DISCONN_COMPLETE starts it
 * again if that device's next link is lost before its stream */
void hci_repage_timer(void)
{
    if (bt->repages >= REPAGE_STOP)
        return;
    if (bt->link.state == LINK_NONE && bt->scan == SCAN_IDLE) {
        logf("bt: link lost: paging again (%u)", ++bt->repages);
        hci_connect(&bt->lost_addr);
    }
    bt_timer(T_REPAGE, REPAGE_PERIOD);
}

/* T_RESTART: the controller did not close the link (no Disconnection
 * Complete within CLOSE_WAIT, or it refused the Disconnect), or has one
 * that we do not. It may keep that link, and the remote with it: a
 * half-open link that no supervision timeout ends and that neither side
 * pages. Restart the controller, which drops it, and page the device again
 * if the link was dropped as silent. */
void hci_restart_timer(void)
{
    struct bt_link *l = &bt->link;
    logf("bt: the controller kept a link: restarting it");
    if (l->state == LINK_CLOSING && (l->flags & LF_SILENT))
        link_lost();
    link_forget();
    bt_restart();
}

/* T_SEC: security took too long, or a retry is due */
void hci_sec_timer(void)
{
    if (bt->link.flags & LF_SEC_BUSY) {
        logf("bt: security timed out");
        bt->link.flags &= ~LF_SEC_BUSY;
        l2cap_link_secured(false);
    } else {
        hci_secure();
    }
}

/* Inquiry */

/* After the inquiry: ask the next found device without a name for it,
 * one at a time; names already known are filled in from our lists */
static void name_next(void)
{
    while (bt->scan == SCAN_NAMES) {
        struct bt_found *f = NULL;
        for (int i = 0; i < bt->nfound && !f; i++)
            if (!bt->found[i].dev.name[0] &&
                !(bt->found[i].psrm & PSRM_NAME_ASKED))
                f = &bt->found[i];
        if (!f) {
            bt->scan = SCAN_IDLE;
            break;
        }
        const char *name = bt_name_of(&f->dev.addr);
        f->psrm |= PSRM_NAME_ASKED;
        if (name) {
            strlcpy(f->dev.name, name, sizeof f->dev.name);
        } else {
            name_request(&f->dev.addr, f->psrm & PSRM_MASK, f->clock);
            break;
        }
    }
}

void hci_scan(bool start)
{
    /* LAP, length, responses (0: unlimited) */
    static const uint8_t inquiry[5] = {
        GIAC_LAP & 0xff, (GIAC_LAP >> 8) & 0xff, GIAC_LAP >> 16,
        INQUIRY_LENGTH, 0
    };
    if (start) {
        if (bt->link.state == LINK_PAGING) {
            bt->scan = SCAN_IDLE;
            return;
        }
        bt->nfound = 0;
        bt->scan = SCAN_INQUIRY;
        hci_cmd(OP_INQUIRY, inquiry, sizeof inquiry);
    } else if (bt->scan == SCAN_INQUIRY) {
        hci_cmd(OP_INQUIRY_CANCEL, NULL, 0);
        bt->scan = SCAN_IDLE;
    } else {
        bt->scan = SCAN_IDLE;   /* the name request under way finishes */
    }
}

/* Add or update an inquiry result; the name comes from EIR if present */
static void found_add(const uint8_t *addr, uint8_t psrm, const uint8_t *cod,
                      uint16_t clock, const uint8_t *eir, size_t eir_len)
{
    struct bt_found *f = found_find(addr);
    if (!f) {
        if (bt->nfound == BT_MAX_FOUND)
            return;
        f = &bt->found[bt->nfound++];
        memset(f, 0, sizeof *f);
        memcpy(&f->dev.addr, addr, BD_ADDR_LEN);
    }
    f->psrm = psrm & PSRM_MASK;
    f->clock = clock | CLOCK_OFFSET_VALID;
    f->dev.cod = cod[0] | cod[1] << 8 | cod[2] << 16;
    /* EIR: [length][type][data], length counting type and data */
    for (size_t i = 0; i + 1 < eir_len && eir[i]; i += eir[i] + 1) {
        size_t n = eir[i] - 1;
        if ((eir[i + 1] == EIR_NAME_FULL || eir[i + 1] == EIR_NAME_SHORT) &&
            i + 2 + n <= eir_len) {
            n = MIN(n, sizeof f->dev.name - 1);
            memcpy(f->dev.name, eir + i + 2, n);
            f->dev.name[n] = '\0';
        }
    }
}

/* The three inquiry result events; p and len cover the parameters */
static void inquiry_result(uint8_t code, const uint8_t *p, size_t len)
{
    unsigned n = p[0];
    p++;
    if (code == EV_INQUIRY_RESULT_EXT) {
        /* one response: BD_ADDR, PSRM, reserved, CoD (3), clock offset
         * (2), RSSI, EIR */
        if (len >= 15)
            found_add(p, p[6], p + 8, get_le16(p + 11), p + 14, len - 15);
        return;
    }
    /* arrays per field: address, PSRM, reserved (1 or 2), CoD, clock
     * (and RSSI, or a second reserved byte above) */
    unsigned rsv = code == EV_INQUIRY_RESULT ? 2 : 1;
    if (len < 1 + n * INQUIRY_RESP_LEN)
        return;
    for (unsigned i = 0; i < n; i++)
        found_add(p + 6 * i, p[6 * n + i], p + (7 + rsv) * n + 3 * i,
                  get_le16(p + (10 + rsv) * n + 2 * i), NULL, 0);
}

/* Events */

/* Command Status: only failures matter; undo what waited for the command */
static void cmd_status(uint16_t op, uint8_t status)
{
    if (!status)
        return;
    logf("bt: command %04x failed: %02x", op, status);
    switch (op) {
    case OP_INQUIRY:
        bt->scan = SCAN_IDLE;
        break;
    case OP_REMOTE_NAME:
        name_next();
        break;
    case OP_CREATE_CONN:
    case OP_ACCEPT_CONN:
        if (bt->link.state == LINK_PAGING)
            page_failed(status);
        break;
    case OP_AUTH_REQUESTED:
    case OP_SET_ENCRYPTION:
        sec_failed(status);
        break;
    case OP_DISCONNECT:
        /* the controller may keep the link: T_RESTART */
        if (bt->link.state == LINK_CLOSING)
            bt_timer(T_RESTART, 1);
        break;
    }
}

/* An event from the RX ring (p after the H4 indicator):
 * [code][parameter length][parameters] */
void hci_event(const uint8_t *p, size_t len)
{
    struct bt_link *l = &bt->link;
    uint8_t code = p[0];
    uint8_t buf[BD_ADDR_LEN + 1 + PIN_LEN];     /* the longest reply */

    if (len < 2 || p[1] + 2u > len)
        return;
    len = p[1];
    p += 2;
    if (code == EV_CMD_COMPLETE) {
        /* credits, opcode (2), return parameters (status first) */
        if (len >= 4 && !bt->up && get_le16(p + 1) == bt->init_op)
            init_complete(get_le16(p + 1), p[3], p + 4, len - 4);
        return;
    }
    if (code == EV_CMD_STATUS) {
        /* status, credits, opcode (2) */
        if (len >= 4)
            cmd_status(get_le16(p + 2), p[0]);
        return;
    }
    if (!bt->up)
        return;

    switch (code) {
    case EV_INQUIRY_RESULT:
    case EV_INQUIRY_RESULT_RSSI:
    case EV_INQUIRY_RESULT_EXT:
        if (len && bt->scan == SCAN_INQUIRY)
            inquiry_result(code, p, len);
        break;
    case EV_INQUIRY_COMPLETE:
        if (bt->scan == SCAN_INQUIRY) {
            bt->scan = SCAN_NAMES;
            name_next();
        }
        break;
    case EV_REMOTE_NAME:            /* status, BD_ADDR, name */
        if (len >= 7) {
            if (!p[0])
                bt_name_learned((const bt_addr_t *)(p + 1), (const char *)p + 7,
                                len - 7);
            name_next();
        }
        break;
    case EV_CONN_REQUEST:           /* BD_ADDR, CoD (3), link type */
        if (len < 10)
            break;
        memcpy(buf, p, BD_ADDR_LEN);
        if (p[9] == LINK_TYPE_ACL && l->state == LINK_NONE) {
            link_init(p, LF_PERIPHERAL);
            l->cod = p[6] | p[7] << 8 | p[8] << 16;
            buf[6] = ROLE_PERIPHERAL;
            hci_cmd(OP_ACCEPT_CONN, buf, 7);
        } else {
            buf[6] = ERR_LIMITED_RESOURCES;
            hci_cmd(OP_REJECT_CONN, buf, 7);
        }
        break;
    case EV_CONN_COMPLETE:
        /* status, handle (2), BD_ADDR, link type, encryption enabled */
        if (len < 11 || p[9] != LINK_TYPE_ACL || l->state != LINK_PAGING ||
            memcmp(&l->addr, p + 3, BD_ADDR_LEN))
            break;
        if (p[0]) {
            logf("bt: connection failed: %02x", p[0]);
            page_failed(p[0]);
            break;
        }
        l->handle = get_le16(p + 1) & HANDLE_MASK;
        l->state = LINK_UP;
        bt_timer(T_REPAGE, 0);      /* any link up ends the re-paging */
        set_connectable(false);
        if (p[10])
            l->flags |= LF_AUTH | LF_ENCRYPT;
        logf("bt: connected, handle %u", l->handle);
        set_supervision_timeout();
        l2cap_link_up();
        if (!l->name[0]) {
            struct bt_found *f = found_find(p + 3);
            name_request(&l->addr, f ? f->psrm & PSRM_MASK : PSRM_DEFAULT,
                         f ? f->clock : 0);
        }
        if (l->flags & LF_OUTGOING)
            hci_secure();
        break;
    case EV_DISCONN_COMPLETE:       /* status, handle (2), reason */
        if (len >= 4 && !p[0] && l->state >= LINK_UP &&
            (get_le16(p + 1) & HANDLE_MASK) == l->handle) {
            logf("bt: disconnected: %02x", p[3]);
            if (p[3] == ERR_CONN_TIMEOUT || p[3] == ERR_LMP_TIMEOUT ||
                (l->flags & LF_SILENT))
                link_lost();
            link_down();
        }
        break;
    case EV_AUTH_COMPLETE:          /* status, handle (2) */
        bt->code[0] = '\0';
        if (len < 3 || l->state != LINK_UP)
            break;
        if (p[0]) {
            sec_failed(p[0]);
        } else {
            l->flags = (l->flags & ~LF_SEC_BUSY) | LF_AUTH;
            hci_secure();
        }
        break;
    case EV_ENCRYPT_CHANGE:         /* status, handle (2), enabled */
        if (len < 4 || l->state != LINK_UP)
            break;
        if (p[0]) {
            sec_failed(p[0]);
        } else if (p[3]) {
            l->flags = (l->flags & ~LF_SEC_BUSY) | LF_AUTH | LF_ENCRYPT;
            bt_timer(T_SEC, 0);
            bt_key_used(&l->addr);
            l2cap_link_secured(true);
        } else {
            l->flags &= ~LF_ENCRYPT;
        }
        break;
    case EV_LINK_KEY_REQUEST: {     /* BD_ADDR; reply BD_ADDR, key */
        struct bt_key *k = len >= BD_ADDR_LEN ?
                           bt_key_find((const bt_addr_t *)p) : NULL;
        if (!k) {
            hci_cmd(OP_LINK_KEY_NEG_REPLY, p, BD_ADDR_LEN);
            break;
        }
        memcpy(buf, p, BD_ADDR_LEN);
        memcpy(buf + BD_ADDR_LEN, k->key, LINK_KEY_LEN);
        hci_cmd(OP_LINK_KEY_REPLY, buf, BD_ADDR_LEN + LINK_KEY_LEN);
        break;
    }
    case EV_LINK_KEY_NOTIFY:        /* BD_ADDR, key, key type */
        if (len >= BD_ADDR_LEN + LINK_KEY_LEN + 1) {
            bt_key_store((const bt_addr_t *)p, p + BD_ADDR_LEN,
                         p[BD_ADDR_LEN + LINK_KEY_LEN]);
            if (!memcmp(&l->addr, p, BD_ADDR_LEN))     /* paired on it */
                l->flags = (l->flags | LF_AUTH) & ~LF_BONDED;
        }
        break;
    case EV_PIN_REQUEST:            /* BD_ADDR; reply BD_ADDR, length, PIN */
        if (HID_KEYBOARD(l->cod))       /* to be typed on it too */
            strlcpy(bt->code, LEGACY_PIN, sizeof bt->code);
        memset(buf, 0, sizeof buf);
        memcpy(buf, p, BD_ADDR_LEN);
        buf[6] = sizeof LEGACY_PIN - 1;
        memcpy(buf + 7, LEGACY_PIN, sizeof LEGACY_PIN - 1);
        hci_cmd(OP_PIN_REPLY, buf, BD_ADDR_LEN + 1 + PIN_LEN);
        break;
    case EV_IO_CAP_REQUEST: {       /* BD_ADDR */
        /* a keyboard's user types the passkey that we show (Passkey
         * Entry); other devices pair with Just Works */
        bool kbd = HID_KEYBOARD(l->cod);
        memcpy(buf, p, BD_ADDR_LEN);
        buf[6] = kbd ? IO_CAP_DISPLAY : IO_CAP_NO_IO;
        buf[7] = OOB_NONE;
        buf[8] = kbd ? AUTH_MITM_BONDING : AUTH_GENERAL_BONDING;
        hci_cmd(OP_IO_CAP_REPLY, buf, 9);
        break;
    }
    case EV_USER_CONFIRM:           /* BD_ADDR, value: accept it */
        cmd_addr(OP_USER_CONFIRM_REPLY, (const bt_addr_t *)p);
        break;
    case EV_USER_PASSKEY:           /* we cannot enter one */
        cmd_addr(OP_PASSKEY_NEG_REPLY, (const bt_addr_t *)p);
        break;
    case EV_PASSKEY_NOTIFY:         /* BD_ADDR, passkey (4) */
        if (len >= BD_ADDR_LEN + 4)
            snprintf(bt->code, sizeof bt->code, "%06lu",
                     (unsigned long)get_le32(p + BD_ADDR_LEN) % 1000000);
        break;
    case EV_SSP_COMPLETE:           /* status, BD_ADDR */
        logf("bt: pairing %s", p[0] ? "failed" : "done");
        bt->code[0] = '\0';
        break;
    case EV_HW_ERROR:
        logf("bt: controller hardware error %02x", p[0]);
        break;
    case EV_ROLE_CHANGE:            /* status, BD_ADDR, new role */
        if (len < 8 || l->state == LINK_NONE ||
            memcmp(&l->addr, p + 1, BD_ADDR_LEN))
            break;
        if (!p[0])
            l->flags = p[7] == ROLE_CENTRAL ? l->flags & ~LF_PERIPHERAL
                                            : l->flags | LF_PERIPHERAL;
        logf("bt: role change %02x: %s", p[0],
             l->flags & LF_PERIPHERAL ? "peripheral" : "central");
        if (!p[0] && p[7] == ROLE_CENTRAL)
            set_supervision_timeout();
        break;
    }
}
