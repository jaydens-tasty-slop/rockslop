/* SPDX-License-Identifier: GPL-2.0-or-later */
/* RFCOMM (TS 07.10 subset, RFCOMM spec 1.2): one multiplexer session on
 * an L2CAP channel and one data channel (DLC) in it, for hfp.c. The
 * headset may start the session and open our server channel
 * (HFP_AG_CHANNEL), or rfcomm_connect() starts one to its channel.
 * Credit based flow control when the remote takes it; we grant credits
 * on our own frames as they run low. Other DLCs are refused, multiplexer
 * commands we do not use are answered as the spec asks. */
#include "bt.h"
#include <string.h>
#include "system.h"

/* Frame: address, control, length (1 or 2 bytes), [credits], information,
 * FCS. Address: EA | C/R << 1 | DLCI << 2. */
#define EA              0x01
#define CR              0x02
#define DLCI_SHIFT      2
#define PF              0x10    /* control: poll/final */
#define SABM            0x2f
#define UA              0x63
#define DM              0x0f
#define DISC            0x43
#define UIH             0xef
#define LEN_SHORT_MAX   127     /* one length byte: len << 1 | EA */
#define HDR_MAX         5       /* address, control, 2 length bytes, credits */
#define FCS_LEN         1

/* Multiplexer control messages (on DLCI 0, in UIH frames): type byte
 * EA | C/R << 1 | type << 2 (C/R: command), length byte, values */
#define MCC_TYPE_MASK   0xfc
#define MCC_PN          0x80    /* parameter negotiation */
#define MCC_PSC         0x40    /* power saving */
#define MCC_CLD         0xc0    /* close down the multiplexer */
#define MCC_TEST        0x20
#define MCC_FCON        0xa0
#define MCC_FCOFF       0x60
#define MCC_MSC         0xe0    /* modem status */
#define MCC_NSC         0x10    /* not supported command (response) */
#define MCC_RPN         0x90    /* remote port negotiation */
#define MCC_RLS         0x50    /* remote line status */
#define MCC_HDR         2

/* PN values: DLCI, frame type | convergence layer, priority, T1, N1 (2,
 * little endian), NA, k (initial credits) */
#define PN_LEN          8
#define PN_DLCI_MASK    0x3f
#define PN_CL_MASK      0xf0
#define PN_CFC_REQ      0xf0    /* credit based flow control: asked for */
#define PN_CFC_ACK      0xe0    /*   and agreed */
#define PN_PRIORITY     7
#define PN_K_MASK       0x07
/* MSC values: the DLCI as an address (EA | C/R), V.24 signals */
#define MSC_LEN         2
#define MSC_SIGNALS     0x8d    /* EA | RTC | RTR | DV */
/* RPN values when the remote only asks: 9600 baud, 8N1, no flow control,
 * XON, XOFF, all parameters valid */
#define RPN_LEN         8
static const uint8_t rpn_default[RPN_LEN - 1] = {
    0x03, 0x03, 0x00, 0x11, 0x13, 0xff, 0x3f
};

#define DEFAULT_MTU     127     /* N1 without parameter negotiation */
#define OUR_MTU         LEN_SHORT_MAX
#define RX_CREDITS      7       /* frames we let the remote send ahead */

/* The FCS: CRC-8 (x^8 + x^2 + x + 1, reflected), over the address and
 * control bytes, and the length for frames other than UIH */
static uint8_t fcs(const uint8_t *p, size_t n)
{
    uint8_t crc = 0xff;
    while (n--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = crc & 1 ? (crc >> 1) ^ 0xe0 : crc >> 1;
    }
    return 0xff - crc;
}

/* Address byte; commands carry C/R = initiator, responses its inverse */
static uint8_t address(uint8_t dlci, bool command)
{
    return EA | (command == bt->rfcomm.initiator ? CR : 0) |
           dlci << DLCI_SHIFT;
}

/* SABM, DISC (commands, poll set) or UA, DM (responses, final set) */
static void control_send(uint8_t dlci, uint8_t type)
{
    struct rfcomm *r = &bt->rfcomm;
    uint8_t *o = bt->tx + BT_HDR;
    o[0] = address(dlci, type == SABM || type == DISC);
    o[1] = type | PF;
    o[2] = EA;                          /* no information */
    o[3] = fcs(o, 3);
    l2cap_send(r->l2, 4);
}

uint8_t *rfcomm_data(void)
{
    return bt->tx + BT_HDR + HDR_MAX;
}

/* A UIH frame with the len bytes at rfcomm_data(); on the data channel
 * with flow control, credits ride along once the remote's run low */
static void uih_send(uint8_t dlci, size_t len)
{
    struct rfcomm *r = &bt->rfcomm;
    uint8_t hdr[HDR_MAX];
    size_t h = 0;
    bool data = dlci && r->cfc;

    if (data && len) {
        if (!r->tx_credits) {
            logf("bt: rfcomm: no credits, %u bytes dropped", (unsigned)len);
            return;
        }
        r->tx_credits--;
    }
    hdr[h++] = address(dlci, true);
    hdr[h++] = UIH;
    if (len > LEN_SHORT_MAX) {
        hdr[h++] = len << 1;
        hdr[h++] = len >> 7;
    } else {
        hdr[h++] = len << 1 | EA;
    }
    if (data && r->rx_credits < RX_CREDITS) {
        hdr[1] |= PF;
        hdr[h++] = RX_CREDITS - r->rx_credits;
        r->rx_credits = RX_CREDITS;
    }
    uint8_t *o = bt->tx + BT_HDR;
    memmove(o + h, rfcomm_data(), len);
    memcpy(o, hdr, h);
    o[h + len] = fcs(o, 2);
    l2cap_send(r->l2, h + len + FCS_LEN);
}

/* As frames of at most the remote's N1 (<= OUR_MTU, so one length byte:
 * a frame's header and FCS stay clear of the bytes still to go) */
void rfcomm_send(size_t len)
{
    struct rfcomm *r = &bt->rfcomm;
    uint8_t *d = rfcomm_data();
    if (r->dlc != DLC_OPEN)
        return;
    while (len) {
        size_t n = MIN(len, r->mtu);
        uih_send(r->dlci, n);
        len -= n;
        memmove(d, d + n, len);
    }
}

/* A multiplexer message with n values at rfcomm_data() + MCC_HDR */
static void mcc_send(uint8_t type, bool command, size_t n)
{
    uint8_t *o = rfcomm_data();
    o[0] = type | (command ? CR : 0) | EA;
    o[1] = n << 1 | EA;
    uih_send(0, MCC_HDR + n);
}

static void pn_send(bool command, uint8_t cl, uint8_t k)
{
    struct rfcomm *r = &bt->rfcomm;
    uint8_t *v = rfcomm_data() + MCC_HDR;
    memset(v, 0, PN_LEN);
    v[0] = r->dlci;
    v[1] = cl;
    v[2] = PN_PRIORITY;
    put_le16(v + 4, command ? OUR_MTU : r->mtu);
    v[7] = k;
    mcc_send(MCC_PN, command, PN_LEN);
}

static void msc_send(void)
{
    uint8_t *v = rfcomm_data() + MCC_HDR;
    v[0] = EA | CR | bt->rfcomm.dlci << DLCI_SHIFT;
    v[1] = MSC_SIGNALS;
    mcc_send(MCC_MSC, true, MSC_LEN);
}

/* The data channel is up, by either side: tell the remote our modem
 * status, then hfp.c */
static void dlc_opened(void)
{
    struct rfcomm *r = &bt->rfcomm;
    r->dlc = DLC_OPEN;
    msc_send();
    logf("bt: rfcomm dlci %u open%s", r->dlci, r->cfc ? ", credits" : "");
    hfp_open();
}

/* Session over: the data channel goes with it */
static void session_reset(void)
{
    struct rfcomm *r = &bt->rfcomm;
    bool open = r->dlc == DLC_OPEN;
    memset(r, 0, sizeof *r);
    if (open)
        hfp_closed();
}

/* The remote's N1 from PN values, kept within 1..OUR_MTU (an N1 of 0
 * would never end rfcomm_send()) */
static uint16_t pn_mtu(const uint8_t *v)
{
    return MAX(1, MIN(get_le16(v + 4), OUR_MTU));
}

/* Parameters for the DLCI: from the remote's command (we answer with
 * ours) or its response to ours */
static void pn_rx(const uint8_t *v, size_t n, bool command)
{
    struct rfcomm *r = &bt->rfcomm;
    if (n < PN_LEN)
        return;
    uint8_t dlci = v[0] & PN_DLCI_MASK, cl = v[1] & PN_CL_MASK;
    if (command) {
        if (r->dlc != DLC_NONE)
            return;
        r->dlci = dlci;
        r->cfc = cl == PN_CFC_REQ;
        r->mtu = pn_mtu(v);
        r->tx_credits = v[7] & PN_K_MASK;
        r->rx_credits = r->cfc ? RX_CREDITS : 0;
        pn_send(false, r->cfc ? PN_CFC_ACK : 0, r->rx_credits);
    } else if (r->dlc == DLC_W4_PN && dlci == r->dlci) {
        r->cfc = cl == PN_CFC_ACK;
        r->mtu = pn_mtu(v);
        r->tx_credits = r->cfc ? v[7] & PN_K_MASK : 0;
        r->rx_credits = r->cfc ? RX_CREDITS : 0;
        r->dlc = DLC_W4_UA;
        control_send(r->dlci, SABM);
    }
}

/* A multiplexer message: [type][length][values] */
static void mcc_rx(const uint8_t *p, size_t n)
{
    if (n < MCC_HDR || !(p[1] & EA) || (size_t)(p[1] >> 1) > n - MCC_HDR)
        return;
    uint8_t type = p[0] & MCC_TYPE_MASK;
    bool command = p[0] & CR;
    size_t len = p[1] >> 1;
    const uint8_t *v = p + MCC_HDR;
    uint8_t *o = rfcomm_data() + MCC_HDR;

    if (type == MCC_PN) {
        pn_rx(v, len, command);
        return;
    }
    if (!command)                       /* MSC response and the like */
        return;
    switch (type) {
    case MCC_RPN:
        memmove(o, v, len);
        if (len == 1) {                 /* a query: our settings */
            memcpy(o + 1, rpn_default, sizeof rpn_default);
            len = RPN_LEN;
        }
        mcc_send(type, false, len);
        break;
    case MCC_MSC:
    case MCC_RLS:
    case MCC_TEST:
    case MCC_PSC:
    case MCC_FCON:
    case MCC_FCOFF:
        memmove(o, v, len);             /* answered with the same values */
        mcc_send(type, false, len);
        break;
    case MCC_CLD:
        mcc_send(type, false, 0);
        l2cap_close(bt->rfcomm.l2);
        break;
    default:
        o[0] = p[0];
        mcc_send(MCC_NSC, false, 1);
        break;
    }
}

/* One frame from the session's L2CAP channel */
static void frame_rx(const uint8_t *p, size_t n)
{
    struct rfcomm *r = &bt->rfcomm;
    if (n < 4)
        return;
    uint8_t dlci = p[0] >> DLCI_SHIFT, type = p[1] & ~PF;
    size_t h = 3, len = p[2] >> 1;
    if (!(p[2] & EA)) {
        len |= (size_t)p[3] << 7;
        h = 4;
    }
    uint8_t credits = 0;
    if (type == UIH && (p[1] & PF) && dlci && r->cfc && h < n)
        credits = p[h++];
    if (h + len + FCS_LEN > n ||
        p[h + len] != fcs(p, type == UIH ? 2 : 3))
        return;
    p += h;

    switch (type) {
    case SABM:
        if (!dlci) {
            if (r->state == RS_W4_SABM) {
                r->state = RS_OPEN;
                control_send(0, UA);
            }
        } else if (r->state == RS_OPEN && r->dlc == DLC_NONE &&
                   dlci >> 1 == HFP_AG_CHANNEL) {
            if (r->dlci != dlci) {      /* no parameter negotiation */
                r->dlci = dlci;
                r->cfc = false;
                r->mtu = DEFAULT_MTU;
            }
            control_send(dlci, UA);
            dlc_opened();
        } else {
            control_send(dlci, DM);
        }
        break;
    case UA:
        if (!dlci && r->state == RS_W4_UA) {
            r->state = RS_OPEN;
            r->dlc = DLC_W4_PN;
            pn_send(true, PN_CFC_REQ, RX_CREDITS);
        } else if (dlci && dlci == r->dlci && r->dlc == DLC_W4_UA) {
            dlc_opened();
        }
        break;
    case DM:
        if (!dlci || (dlci == r->dlci && r->dlc != DLC_OPEN)) {
            logf("bt: rfcomm dlci %u refused", dlci);
            l2cap_close(r->l2);
        }
        break;
    case DISC:
        control_send(dlci, UA);
        if (!dlci) {
            l2cap_close(r->l2);
        } else if (dlci == r->dlci && r->dlc == DLC_OPEN) {
            r->dlc = DLC_NONE;
            hfp_closed();
            if (r->initiator)
                l2cap_close(r->l2);
        }
        break;
    case UIH:
        if (!dlci) {
            mcc_rx(p, len);
        } else if (dlci == r->dlci && r->dlc == DLC_OPEN) {
            r->tx_credits = MIN(r->tx_credits + credits, 255);
            if (len && r->cfc && r->rx_credits)
                r->rx_credits--;
            if (len)
                hfp_rx(p, len);
            if (r->cfc && r->rx_credits <= RX_CREDITS / 2)
                uih_send(dlci, 0);      /* more credits */
        }
        break;
    }
}

/* Start a session to the remote's server channel; hfp_open() follows */
bool rfcomm_connect(uint8_t channel)
{
    struct rfcomm *r = &bt->rfcomm;
    if (r->l2 || !(r->l2 = l2cap_open(PSM_RFCOMM)))
        return false;
    logf("bt: rfcomm connect to channel %u", channel);
    r->state = RS_W4_L2CAP;
    r->initiator = true;
    r->dlci = channel << 1;     /* the responder's channel, from the initiator */
    return true;
}

/* L2CAP events: one session; a second channel is closed */
void rfcomm_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len)
{
    struct rfcomm *r = &bt->rfcomm;
    switch (ev) {
    case CHAN_OPEN:
        if (r->l2 && c != r->l2) {
            l2cap_close(c);
        } else if (r->state == RS_W4_L2CAP) {   /* ours */
            r->state = RS_W4_UA;
            control_send(0, SABM);
        } else {                                /* the headset's */
            r->l2 = c;
            r->state = RS_W4_SABM;
        }
        break;
    case CHAN_DATA:
        if (c == r->l2)
            frame_rx(p, len);
        break;
    case CHAN_CLOSED:
        if (c == r->l2)
            session_reset();
        break;
    }
}
