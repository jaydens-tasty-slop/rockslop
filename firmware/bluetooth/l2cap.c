/* SPDX-License-Identifier: GPL-2.0-or-later */
/* L2CAP basic mode (Core spec Vol 3, Part A): signalling and channels on
 * the one ACL link. Channels other than SDP need an encrypted link; an
 * inbound request waits ("pending") while the link is secured.
 *
 * A channel opens once both directions are configured: we only ask for
 * our MTU (BT_MTU) and accept the remote's options as long as they mean
 * basic mode. Opens, data and closes go to the PSM's layer (chan_event). */
#include "bt.h"
#include <string.h>

#define SIG_CID         0x0001  /* signalling channel */
#define CID_DYN_FIRST   0x0040  /* first dynamically allocated CID */
#define SIG_HDR         4       /* command: code, identifier, length (2) */

/* Signalling command codes */
#define SIG_REJECT      0x01
#define SIG_CONN_REQ    0x02
#define SIG_CONN_RSP    0x03
#define SIG_CONF_REQ    0x04
#define SIG_CONF_RSP    0x05
#define SIG_DISC_REQ    0x06
#define SIG_DISC_RSP    0x07
#define SIG_ECHO_REQ    0x08
#define SIG_ECHO_RSP    0x09
#define SIG_INFO_REQ    0x0a
#define SIG_INFO_RSP    0x0b

/* Command reject reasons */
#define REJ_NOT_UNDERSTOOD  0x0000
#define REJ_INVALID_CID     0x0002

/* Connection response results */
#define CONN_OK         0
#define CONN_PENDING    1
#define CONN_BAD_PSM    2
#define CONN_SECURITY   3
#define CONN_NO_RES     4
/* Connection response status, with CONN_PENDING */
#define CONN_NO_INFO    0
#define CONN_AUTHEN     1       /* authentication pending */

/* Configuration: request [DCID][flags][options], response
 * [SCID][flags][result][options]; option [type][length][value] */
#define CONF_REQ_HDR    4
#define CONF_RSP_HDR    6
#define CONF_RSP_MAX    32      /* stop listing unknown types at this size */
#define CONF_CONT       0x0001  /* flags: continuation */
#define CONF_SUCCESS    0
#define CONF_UNACCEPT   1       /* unacceptable parameters */
#define CONF_UNKNOWN    3       /* unknown options */
#define OPT_HDR         2
#define OPT_TYPE        0x7f    /* type byte: option type, */
#define OPT_HINT        0x80    /* and hint (skip if unknown) */
#define OPT_MTU         0x01
#define OPT_FLUSH_TO    0x02    /* flush timeout */
#define OPT_QOS         0x03
#define OPT_RFC         0x04    /* retransmission and flow control */
#define OPT_EXT_WINDOW  0x07    /* extended window size */
#define OPT_MTU_LEN     2
#define OPT_RFC_LEN     9
#define RFC_BASIC       0x00    /* RFC mode */

/* Information requests */
#define INFO_HDR        4       /* response: type, result */
#define INFO_CL_MTU     0x0001  /* connectionless MTU */
#define INFO_FEATURES   0x0002  /* extended features mask */
#define INFO_FIXED_CH   0x0003  /* fixed channels supported */
#define INFO_SUCCESS    0x0000
#define INFO_NOT_SUPP   0x0001
#define FIXED_CH_SIG    0x02    /* fixed channels mask: bit 1, CID 0x0001 */

#define DEFAULT_MTU     672     /* the remote's MTU until it configures one */
#define MIN_MTU         48
#define ECHO_MAX        64u     /* echo request data we return */

/* Signalling payload goes after the 4-byte command header */
static uint8_t *sig_data(void)
{
    return bt->tx + BT_HDR + SIG_HDR;
}

/* Send the command with len bytes at sig_data() */
static void sig_send(uint8_t code, uint8_t ident, size_t len)
{
    uint8_t *p = bt->tx + BT_HDR;
    p[0] = code;
    p[1] = ident;
    put_le16(p + 2, len);
    acl_send(bt->tx, SIG_CID, len + SIG_HDR, false);
}

/* Identifiers for our requests: 1..255, 0 is invalid */
static uint8_t next_ident(void)
{
    if (!++bt->link.ident)
        bt->link.ident = 1;
    return bt->link.ident;
}

/* Hand a channel event to the layer that owns the PSM */
static void chan_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len)
{
    switch (c->psm) {
    case PSM_SDP:
        sdp_event(c, ev, p, len);
        break;
    case PSM_AVDTP:
        avdtp_event(c, ev, p, len);
        break;
    case PSM_AVCTP:
        avrcp_event(c, ev, p, len);
        break;
    case PSM_RFCOMM:
        rfcomm_event(c, ev, p, len);
        break;
#ifdef HAVE_BLUETOOTH_HID
    case PSM_HID_CTRL:
    case PSM_HID_INTR:
        hid_event(c, ev, p, len);
        break;
#endif
    }
}

static struct l2cap_chan *chan_find(uint16_t lcid)
{
    for (int i = 0; i < BT_CHANNELS; i++)
        if (lcid && bt->chan[i].lcid == lcid)
            return &bt->chan[i];
    return NULL;
}

/* A free channel slot. Local CIDs rotate instead of following the slot:
 * l2cap_close does not wait for the disconnection response, and the XM3
 * silently drops a Connection Request whose source CID it is still
 * disconnecting (the HFP RFCOMM channel right after its SDP query). */
static struct l2cap_chan *chan_new(uint16_t psm)
{
    for (int i = 0; i < BT_CHANNELS; i++) {
        struct l2cap_chan *c = &bt->chan[i];
        if (!c->lcid) {
            uint16_t cid;
            do {
                cid = MAX(bt->next_cid, CID_DYN_FIRST);
                bt->next_cid = cid + 1;     /* wraps to 0: CID_DYN_FIRST */
            } while (chan_find(cid));
            memset(c, 0, sizeof *c);
            c->lcid = cid;
            c->psm = psm;
            c->rmtu = DEFAULT_MTU;
            return c;
        }
    }
    return NULL;
}

/* Free the slot and tell the upper layer */
static void chan_free(struct l2cap_chan *c)
{
    c->lcid = 0;
    chan_event(c, CHAN_CLOSED, NULL, 0);
}

static void send_conn_rsp(uint8_t ident, uint16_t lcid, uint16_t rcid,
                          uint16_t result, uint16_t status)
{
    uint8_t *d = sig_data();
    put_le16(d, lcid);
    put_le16(d + 2, rcid);
    put_le16(d + 4, result);
    put_le16(d + 6, status);
    sig_send(SIG_CONN_RSP, ident, 8);
}

/* Our configuration: just the MTU */
static void send_conf_req(struct l2cap_chan *c)
{
    uint8_t *d = sig_data();
    c->state = CH_CONFIG;
    put_le16(d, c->rcid);
    put_le16(d + 2, 0);
    d[4] = OPT_MTU;
    d[5] = OPT_MTU_LEN;
    put_le16(d + 6, BT_MTU);
    c->ident = next_ident();
    sig_send(SIG_CONF_REQ, c->ident, CONF_REQ_HDR + OPT_HDR + OPT_MTU_LEN);
}

/* Accept the remote's connection request and start configuring */
static void accept_chan(struct l2cap_chan *c)
{
    send_conn_rsp(c->ident, c->lcid, c->rcid, CONN_OK, CONN_NO_INFO);
    send_conf_req(c);
}

/* Do we take an inbound channel for this PSM now? */
static bool psm_wanted(uint16_t psm)
{
    switch (psm) {
    case PSM_SDP:
        return true;
    case PSM_AVDTP:
        return avdtp_accept();
    case PSM_AVCTP:
        return !bt->avctp;
    case PSM_RFCOMM:
        return !bt->rfcomm.l2;
#ifdef HAVE_BLUETOOTH_HID
    case PSM_HID_CTRL:
    case PSM_HID_INTR:
        return hid_accept(psm);
#endif
    }
    return false;
}

/* Connection request: [PSM][source CID]. Secure the link first unless
 * the channel is SDP or the link is encrypted already. */
static void conn_req(uint8_t id, const uint8_t *d)
{
    uint16_t psm = get_le16(d), rcid = get_le16(d + 2);
    uint16_t result = CONN_BAD_PSM;
    struct l2cap_chan *c;

    if (psm_wanted(psm)) {
        result = CONN_NO_RES;
        if ((c = chan_new(psm))) {
            c->rcid = rcid;
            c->ident = id;
            if (psm == PSM_SDP || (bt->link.flags & LF_ENCRYPT)) {
                accept_chan(c);
            } else {
                c->state = CH_W4_SEC;
                send_conn_rsp(id, c->lcid, rcid, CONN_PENDING, CONN_AUTHEN);
                hci_secure();
            }
            return;
        }
    }
    send_conn_rsp(id, 0, rcid, result, CONN_NO_INFO);
}

/* Open once both directions are configured */
static void check_open(struct l2cap_chan *c)
{
    if ((c->flags & (CHF_IN_DONE | CHF_OUT_DONE)) == (CHF_IN_DONE | CHF_OUT_DONE)) {
        c->state = CH_OPEN;
        logf("bt: l2cap psm %04x open, cid %04x mtu %u", c->psm, c->lcid, c->rmtu);
        chan_event(c, CHAN_OPEN, NULL, 0);
    }
}

/* The remote's configuration request. We take its MTU (at least
 * MIN_MTU) and anything that keeps basic mode; the response lists the
 * unknown options, else the values we want instead (each once, however
 * often the request repeats an option). */
static void conf_req(uint8_t id, const uint8_t *d, size_t n)
{
    struct l2cap_chan *c = chan_find(get_le16(d));
    uint16_t flags = get_le16(d + 2), result = CONF_SUCCESS;
    uint8_t *o = sig_data(), *out = o + CONF_RSP_HDR;
    bool mtu_low = false, not_basic = false;

    if (!c || c->state != CH_CONFIG) {
        /* data: the CID asked for, and the remote's (not known here) */
        put_le16(o, REJ_INVALID_CID);
        put_le16(o + 2, get_le16(d));
        put_le16(o + 4, 0);
        sig_send(SIG_REJECT, id, 6);
        return;
    }
    for (size_t i = CONF_REQ_HDR; i + OPT_HDR <= n &&
         i + OPT_HDR + d[i + 1] <= n; i += OPT_HDR + d[i + 1]) {
        const uint8_t *opt = d + i;
        switch (opt[0] & OPT_TYPE) {
        case OPT_MTU:
            if (opt[1] >= OPT_MTU_LEN) {
                c->rmtu = get_le16(opt + 2);
                mtu_low = c->rmtu < MIN_MTU;
            }
            break;
        case OPT_FLUSH_TO:
        case OPT_QOS:                   /* best effort is fine */
        case OPT_EXT_WINDOW:            /* ERTM and streaming mode only */
            break;
        case OPT_RFC:
            if (opt[1] >= 1)
                not_basic = opt[2] != RFC_BASIC;
            break;
        default:
            if (!(opt[0] & OPT_HINT) && out < o + CONF_RSP_MAX) {
                result = CONF_UNKNOWN;  /* listed by type */
                *out++ = opt[0];
            }
            break;
        }
    }
    if (!result && (mtu_low || not_basic)) {
        result = CONF_UNACCEPT;
        if (mtu_low) {
            out[0] = OPT_MTU;
            out[1] = OPT_MTU_LEN;
            put_le16(out + OPT_HDR, MIN_MTU);
            out += OPT_HDR + OPT_MTU_LEN;
        }
        if (not_basic) {                /* ask for basic mode (all zero) */
            memset(out, 0, OPT_HDR + OPT_RFC_LEN);
            out[0] = OPT_RFC;
            out[1] = OPT_RFC_LEN;
            out += OPT_HDR + OPT_RFC_LEN;
        }
    }
    put_le16(o, c->rcid);
    put_le16(o + 2, flags & CONF_CONT);
    put_le16(o + 4, result);
    sig_send(SIG_CONF_RSP, id, out - o);
    if (!result && !(flags & CONF_CONT)) {
        c->flags |= CHF_IN_DONE;
        check_open(c);
    }
}

/* The response to our configuration request: [SCID][flags][result] */
static void conf_rsp(const uint8_t *d)
{
    struct l2cap_chan *c = chan_find(get_le16(d));
    if (!c || c->state != CH_CONFIG)
        return;
    if (get_le16(d + 4) == CONF_SUCCESS) {
        c->flags |= CHF_OUT_DONE;
        check_open(c);
    } else {
        l2cap_close(c);
    }
}

static void info_req(uint8_t id, uint16_t type)
{
    uint8_t *o = sig_data();
    size_t n = INFO_HDR;
    put_le16(o, type);
    put_le16(o + 2, INFO_SUCCESS);
    switch (type) {
    case INFO_CL_MTU:
        put_le16(o + 4, MIN_MTU);
        n += 2;
        break;
    case INFO_FEATURES:                 /* none */
        memset(o + 4, 0, 4);
        n += 4;
        break;
    case INFO_FIXED_CH:                 /* signalling only */
        memset(o + 4, 0, 8);
        o[4] = FIXED_CH_SIG;
        n += 8;
        break;
    default:
        put_le16(o + 2, INFO_NOT_SUPP);
        break;
    }
    sig_send(SIG_INFO_RSP, id, n);
}

/* One signalling command: d and n are its data */
static void sig_command(uint8_t code, uint8_t id, const uint8_t *d, size_t n)
{
    struct l2cap_chan *c;
    uint8_t *o = sig_data();

    switch (code) {
    case SIG_CONN_REQ:
        if (n >= 4)
            conn_req(id, d);
        break;
    case SIG_CONN_RSP:              /* [DCID][SCID][result][status] */
        if (n < 8 || !(c = chan_find(get_le16(d + 2))) || c->state != CH_W4_CONN)
            break;
        if (get_le16(d + 4) == CONN_OK) {
            c->rcid = get_le16(d);
            send_conf_req(c);
        } else if (get_le16(d + 4) != CONN_PENDING) {
            logf("bt: l2cap psm %04x refused: %u", c->psm, get_le16(d + 4));
            chan_free(c);
        }
        break;
    case SIG_CONF_REQ:
        if (n >= CONF_REQ_HDR)
            conf_req(id, d, n);
        break;
    case SIG_CONF_RSP:
        if (n >= CONF_RSP_HDR)
            conf_rsp(d);
        break;
    case SIG_DISC_REQ:              /* [DCID][SCID], echoed in the response */
        if (n < 4)
            break;
        memcpy(o, d, 4);
        sig_send(SIG_DISC_RSP, id, 4);
        if ((c = chan_find(get_le16(d))) && c->rcid == get_le16(d + 2))
            chan_free(c);
        break;
    case SIG_ECHO_REQ:
        n = MIN(n, ECHO_MAX);
        memcpy(o, d, n);
        sig_send(SIG_ECHO_RSP, id, n);
        break;
    case SIG_INFO_REQ:              /* [type] */
        if (n >= 2)
            info_req(id, get_le16(d));
        break;
    case SIG_REJECT:                /* our request failed */
        for (int i = 0; i < BT_CHANNELS; i++) {
            c = &bt->chan[i];
            if (c->lcid && c->ident == id &&
                (c->state == CH_W4_CONN || c->state == CH_CONFIG))
                chan_free(c);
        }
        break;
    case SIG_DISC_RSP:
    case SIG_ECHO_RSP:
    case SIG_INFO_RSP:
        break;
    default:
        put_le16(o, REJ_NOT_UNDERSTOOD);
        sig_send(SIG_REJECT, id, 2);
        break;
    }
}

/* A complete L2CAP PDU from the link: [length][CID][payload] */
void l2cap_rx(const uint8_t *pdu, size_t len)
{
    uint16_t cid = get_le16(pdu + 2);
    if ((size_t)get_le16(pdu) + L2CAP_HDR != len)
        return;
    pdu += L2CAP_HDR;
    len -= L2CAP_HDR;
    if (cid == SIG_CID) {
        while (len >= SIG_HDR) {        /* one or more commands */
            size_t n = get_le16(pdu + 2);
            if (n + SIG_HDR > len)
                break;
            sig_command(pdu[0], pdu[1], pdu + SIG_HDR, n);
            pdu += SIG_HDR + n;
            len -= SIG_HDR + n;
        }
        return;
    }
    struct l2cap_chan *c = chan_find(cid);
    if (c && c->state == CH_OPEN)
        chan_event(c, CHAN_DATA, pdu, len);
}

/* Open a channel to the remote's PSM; CHAN_OPEN or CHAN_CLOSED follows */
struct l2cap_chan *l2cap_open(uint16_t psm)
{
    struct l2cap_chan *c = chan_new(psm);
    if (!c)
        return NULL;
    uint8_t *d = sig_data();
    c->state = CH_W4_CONN;
    c->ident = next_ident();
    put_le16(d, psm);
    put_le16(d + 2, c->lcid);
    sig_send(SIG_CONN_REQ, c->ident, 4);
    return c;
}

/* Close a channel, with a disconnection request if it got as far as
 * configuration; the response is not awaited */
void l2cap_close(struct l2cap_chan *c)
{
    if (!c->lcid)
        return;
    if (c->state >= CH_CONFIG) {
        uint8_t *d = sig_data();
        put_le16(d, c->rcid);
        put_le16(d + 2, c->lcid);
        sig_send(SIG_DISC_REQ, next_ident(), 4);
    }
    chan_free(c);
}

/* Send the len-byte SDU at bt->tx + BT_HDR, if the channel is open and
 * the remote takes that much */
void l2cap_send(struct l2cap_chan *c, size_t len)
{
    if (c->state == CH_OPEN && len <= c->rmtu)
        acl_send(bt->tx, c->rcid, len, false);
}

void l2cap_link_up(void)
{
    memset(bt->chan, 0, sizeof bt->chan);
    hfp_link_up();
}

/* Security finished: answer the requests that waited for it, or drop
 * the link */
void l2cap_link_secured(bool ok)
{
    for (int i = 0; i < BT_CHANNELS; i++) {
        struct l2cap_chan *c = &bt->chan[i];
        if (!c->lcid || c->state != CH_W4_SEC)
            continue;
        if (ok) {
            accept_chan(c);
        } else {
            send_conn_rsp(c->ident, 0, c->rcid, CONN_SECURITY, CONN_NO_INFO);
            chan_free(c);
        }
    }
    if (!ok)
        hci_disconnect();
#ifdef HAVE_BLUETOOTH_HID
    else if (hid_device())
        hid_link_secured();
#endif
    else
        avdtp_link_secured();
}

void l2cap_link_down(void)
{
    for (int i = 0; i < BT_CHANNELS; i++)
        if (bt->chan[i].lcid)
            chan_free(&bt->chan[i]);
}
