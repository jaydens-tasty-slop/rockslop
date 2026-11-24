/* SPDX-License-Identifier: GPL-2.0-or-later */
/* AVCTP / AVRCP. Target, category 1 (AVRCP 1.3, sdp.c): passthrough keys
 * from the headset become multimedia buttons (apps/misc.c). The play
 * status (GET PLAY STATUS, and its change notification, polled every
 * PLAY_POLL while the headset waits for it) tells a headset whether its
 * play/pause key should play or pause. UNIT INFO and SUBUNIT INFO are
 * answered, other AVRCP PDUs are rejected and other AV/C commands get NOT
 * IMPLEMENTED. Controller, category 2 (AVRCP 1.4):
 * absolute volume, if the headset lists the volume event. One AVCTP
 * channel, opened by the headset or, if it has not once the stream is
 * open, by avrcp_timer().
 *
 * Absolute volume: once the channel is up we ask for the headset's events
 * (GET CAPABILITIES), register for its volume changes and, once the
 * stream is routed to it, send it the Rockbox volume (SET ABSOLUTE
 * VOLUME), but on connecting never more than the headset's own: a volume
 * set for a headset without absolute volume could be far too loud, so
 * Rockbox comes down to the headset's instead. After it accepts one, bt_volume()
 * tells firmware/sound.c to send the PCM at full scale and every volume
 * change goes to the headset instead; the headset's own changes come back
 * as SYS_BT_VOLUME. Rockbox's volume range maps linearly onto 0..127.
 * T_AVRCP times out our commands: each goes twice more (an answer lost on
 * a link at the range edge), then the PCM volume is used again. */
#include "bt.h"
#include <string.h>
#include "audio.h"
#include "button.h"
#include "pcm_sink.h"

/* AVCTP header, single packet: transaction label << 4 | packet type << 2 |
 * C/R << 1 | IPID, then the profile identifier */
#define AVCTP_HDR           3
#define AVCTP_LABEL_SHIFT   4
#define AVCTP_LABEL_MASK    0xf0
#define AVCTP_PKT_TYPE_MASK 0x0c    /* 0: single packet */
#define AVCTP_RSP           0x02    /* C/R: response */
#define AVCTP_IPID          0x01    /* invalid profile identifier */
#define AVRCP_PID           0x110e  /* A/V_RemoteControl service class */
#define LABELS              16

/* AV/C frame after the AVCTP header: ctype or response (low 4 bits),
 * subunit type << 3 | subunit ID, opcode, then operands */
#define AVC_HDR             3
#define AVC_CTYPE           AVCTP_HDR   /* offsets in the AVCTP message */
#define AVC_SUBUNIT         (AVCTP_HDR + 1)
#define AVC_OPCODE          (AVCTP_HDR + 2)
#define AVC_OPERANDS        (AVCTP_HDR + AVC_HDR)
#define AVC_CTYPE_MASK      0x0f
#define AVC_ECHO_MAX        64u     /* frame bytes echoed in a response */

#define CTYPE_CONTROL       0x00
#define CTYPE_STATUS        0x01
#define CTYPE_NOTIFY        0x03
#define CTYPE_RESPONSE      0x08    /* ctypes from here up are responses */
#define RSP_NOT_IMPLEMENTED 0x08
#define RSP_ACCEPTED        0x09
#define RSP_REJECTED        0x0a
#define RSP_STABLE          0x0c
#define RSP_CHANGED         0x0d
#define RSP_INTERIM         0x0f

#define OP_VENDOR           0x00    /* vendor dependent: AVRCP PDUs */
#define OP_UNIT_INFO        0x30
#define OP_SUBUNIT_INFO     0x31
#define OP_PASSTHROUGH      0x7c

#define SUBUNIT_PANEL       0x09

/* Passthrough operands: state flag | operation ID, operation data length */
#define PT_OPERANDS         2
#define PT_RELEASED         0x80    /* state flag: released, else pressed */
#define PT_OP_MASK          0x7f
#define PT_VOLUME_UP        0x41
#define PT_VOLUME_DOWN      0x42
#define PT_PLAY             0x44
#define PT_STOP             0x45
#define PT_PAUSE            0x46
#define PT_REWIND           0x48
#define PT_FAST_FORWARD     0x49
#define PT_FORWARD          0x4b    /* next track */
#define PT_BACKWARD         0x4c    /* previous track */

/* Vendor dependent operands: Bluetooth SIG company ID (3 bytes), PDU ID,
 * packet type (0: single), parameter length (2), parameters */
#define BT_SIG_COMPANY      0x001958
#define VD_HDR              7
#define VD_PDU              (AVC_OPERANDS + 3)
#define VD_PARAMS           (AVC_OPERANDS + VD_HDR)
#define PDU_GET_CAPS        0x10
#define PDU_REGISTER        0x31    /* register notification */
#define PDU_GET_PLAY_STATUS 0x30
#define PDU_SET_VOLUME      0x50    /* set absolute volume */
#define CAP_COMPANY_ID      0x02    /* get capabilities: company IDs */
#define CAP_EVENTS          0x03    /* get capabilities: events supported */
#define EVENT_PLAYBACK      0x01    /* playback status changed */
#define EVENT_VOLUME        0x0d    /* volume changed */
#define VOLUME_MAX          0x7f
#define ERR_INVALID_COMMAND 0x00    /* REJECTED's status */
#define ERR_INVALID_PARAM   0x01

/* Play status: stopped, playing, paused; song length and position */
#define PLAY_STOPPED        0x00
#define PLAY_PLAYING        0x01
#define PLAY_PAUSED         0x02
#define PLAY_UNKNOWN_MS     0xffffffff
#define PLAY_POLL           (HZ / 2)

#define CMD_TIMEOUT         (2 * HZ)    /* our command unanswered */
#define CMD_RETRIES         2           /* ...then sent this often again */

/* The Rockbox volume, from bt_volume() also while Bluetooth is off */
static int vol_cb, vol_min, vol_max;
static bool vol_known;

/* Volume in centibels to 0..VOLUME_MAX and back, linear and rounded both
 * ways (with steps finer than 1/127 of the range, a round trip through
 * the nearest step comes back to the same value) */
static int to_abs(int cb)
{
    int range = vol_max - vol_min;
    if (range <= 0)
        return VOLUME_MAX;
    cb = MIN(MAX(cb, vol_min), vol_max);
    return ((cb - vol_min) * VOLUME_MAX + range / 2) / range;
}

static int from_abs(int v)
{
    return vol_min + (v * (vol_max - vol_min) + VOLUME_MAX / 2) / VOLUME_MAX;
}

/* One passthrough operation; false if we do not support it. Keys act when
 * pressed; play and pause go to the UI thread as states (SYS_BT_PLAY,
 * apps/bluetooth_policy.c), which acts only when they change it. */
static bool passthrough(uint8_t op)
{
    long btn = 0;

    switch (op & PT_OP_MASK) {
    case PT_PLAY:
    case PT_PAUSE:
        logf("bt: avrcp key %02x %s", op & PT_OP_MASK,
             op & PT_RELEASED ? "up" : "down");
        if (!(op & PT_RELEASED))
            button_queue_post(SYS_BT_PLAY, (op & PT_OP_MASK) == PT_PLAY);
        return true;
    case PT_STOP:           btn = BUTTON_MULTIMEDIA_STOP;        break;
    case PT_FORWARD:        btn = BUTTON_MULTIMEDIA_NEXT;        break;
    case PT_BACKWARD:       btn = BUTTON_MULTIMEDIA_PREV;        break;
    case PT_VOLUME_UP:      btn = BUTTON_MULTIMEDIA_VOLUME_UP;   break;
    case PT_VOLUME_DOWN:    btn = BUTTON_MULTIMEDIA_VOLUME_DOWN; break;
    case PT_REWIND:         btn = BUTTON_MULTIMEDIA_REW;         break;
    case PT_FAST_FORWARD:   btn = BUTTON_MULTIMEDIA_FFWD;        break;
    default:
        return false;
    }
    logf("bt: avrcp key %02x %s", op & PT_OP_MASK,
         op & PT_RELEASED ? "up" : "down");
    if (!(op & PT_RELEASED) && btn)
        button_queue_try_post(btn, 0);
    return true;
}

static uint8_t *params(void)
{
    return bt->tx + BT_HDR + VD_PARAMS;
}

/* The play status as AVRCP has it */
static uint8_t play_status(void)
{
    int st = audio_status();
    if (!(st & AUDIO_STATUS_PLAY))
        return PLAY_STOPPED;
    return st & AUDIO_STATUS_PAUSE ? PLAY_PAUSED : PLAY_PLAYING;
}

/* One AVRCP PDU from the headset: the response's parameters go to
 * params(), their length to *np; returns the response ctype */
static uint8_t target_pdu(const uint8_t *p, size_t len, size_t *np)
{
    struct avrcp *r = &bt->avrcp;
    uint8_t ctype = p[AVC_CTYPE] & AVC_CTYPE_MASK, *o = params();
    const uint8_t *a = p + VD_PARAMS;
    size_t n = MIN(get_be16(p + VD_PDU + 2), len - VD_PARAMS);

    *np = 1;
    switch (p[VD_PDU]) {
    case PDU_GET_CAPS:
        if (ctype != CTYPE_STATUS || n < 1)
            break;
        o[0] = a[0];
        o[1] = 1;
        if (a[0] == CAP_COMPANY_ID) {
            o[2] = BT_SIG_COMPANY >> 16;
            put_be16(o + 3, BT_SIG_COMPANY & 0xffff);
            *np = 5;
            return RSP_STABLE;
        }
        if (a[0] == CAP_EVENTS) {
            o[2] = EVENT_PLAYBACK;
            *np = 3;
            return RSP_STABLE;
        }
        o[0] = ERR_INVALID_PARAM;
        return RSP_REJECTED;
    case PDU_GET_PLAY_STATUS:
        if (ctype != CTYPE_STATUS)
            break;
        put_be32(o, PLAY_UNKNOWN_MS);
        put_be32(o + 4, PLAY_UNKNOWN_MS);
        o[8] = play_status();
        *np = 9;
        return RSP_STABLE;
    case PDU_REGISTER:
        if (ctype != CTYPE_NOTIFY || n < 1)
            break;
        if (a[0] != EVENT_PLAYBACK) {
            o[0] = ERR_INVALID_PARAM;
            return RSP_REJECTED;
        }
        r->play_reg = true;
        r->play_label = p[0] >> AVCTP_LABEL_SHIFT;
        o[0] = EVENT_PLAYBACK;
        o[1] = r->play = play_status();
        *np = 2;
        bt_timer(T_PLAY, PLAY_POLL);
        return RSP_INTERIM;
    }
    o[0] = ERR_INVALID_COMMAND;
    return RSP_REJECTED;
}

/* One AVCTP command: answer it, the response echoing the command */
static void target_rx(struct l2cap_chan *c, const uint8_t *p, size_t len)
{
    uint8_t *o = bt->tx + BT_HDR;
    o[0] = (p[0] & AVCTP_LABEL_MASK) | AVCTP_RSP;   /* same label */
    o[1] = p[1];
    o[2] = p[2];
    if (get_be16(p + 1) != AVRCP_PID) {
        o[0] |= AVCTP_IPID;
        l2cap_send(c, AVCTP_HDR);
        return;
    }
    if (len < AVC_OPERANDS)
        return;
    size_t n = MIN(len - AVCTP_HDR, AVC_ECHO_MAX);
    uint8_t ctype = p[AVC_CTYPE] & AVC_CTYPE_MASK, opcode = p[AVC_OPCODE],
            rsp = RSP_NOT_IMPLEMENTED;
    memcpy(o + AVCTP_HDR, p + AVCTP_HDR, n);
    switch (opcode) {
    case OP_UNIT_INFO:
    case OP_SUBUNIT_INFO:
        if (ctype == CTYPE_STATUS) {
            /* UNIT INFO: 7, unit type panel and unit 0, company ID
             * 0xffffff. SUBUNIT INFO: page 0 and extension code 7, then a
             * panel subunit (max ID 0) and three unused entries. */
            static const uint8_t info[5] = {
                0x07, SUBUNIT_PANEL << 3, 0xff, 0xff, 0xff
            };
            memcpy(o + AVC_OPERANDS, info, sizeof info);
            n = AVC_HDR + sizeof info;
            rsp = RSP_STABLE;
        }
        break;
    case OP_PASSTHROUGH:
        if (ctype == CTYPE_CONTROL && len >= AVC_OPERANDS + PT_OPERANDS &&
            passthrough(p[AVC_OPERANDS]))
            rsp = RSP_ACCEPTED;
        break;
    case OP_VENDOR:
        if (len >= VD_PARAMS && p[AVC_OPERANDS] == BT_SIG_COMPANY >> 16 &&
            get_be16(p + AVC_OPERANDS + 1) == (BT_SIG_COMPANY & 0xffff)) {
            size_t np;
            rsp = target_pdu(p, len, &np);
            put_be16(o + VD_PDU + 2, np);
            n = VD_PARAMS - AVCTP_HDR + np;
        }
        break;
    }
    o[AVC_CTYPE] = rsp;
    l2cap_send(c, AVCTP_HDR + n);
}

/* Send an AVRCP PDU with n parameter bytes (from VD_PARAMS in bt->tx) on
 * label, as a command or with a response ctype as a response; returns the
 * label */
static uint8_t vendor_send(uint8_t label, uint8_t ctype, uint8_t pdu, size_t n)
{
    uint8_t *o = bt->tx + BT_HDR;
    o[0] = label << AVCTP_LABEL_SHIFT |
           (ctype & CTYPE_RESPONSE ? AVCTP_RSP : 0);
    put_be16(o + 1, AVRCP_PID);
    o[AVC_CTYPE] = ctype;
    o[AVC_SUBUNIT] = SUBUNIT_PANEL << 3;
    o[AVC_OPCODE] = OP_VENDOR;
    o[AVC_OPERANDS] = BT_SIG_COMPANY >> 16;
    put_be16(o + AVC_OPERANDS + 1, BT_SIG_COMPANY & 0xffff);
    o[VD_PDU] = pdu;
    o[VD_PDU + 1] = 0;
    put_be16(o + VD_PDU + 2, n);
    if (bt->avctp)
        l2cap_send(bt->avctp, VD_PARAMS + n);
    return label;
}

/* A command that waits for its response: one at a time, never on the
 * notification's label */
static void command(uint8_t ctype, uint8_t pdu, size_t n)
{
    struct avrcp *r = &bt->avrcp;
    if (pdu != r->pending)
        r->retries = 0;
    if (r->label == r->nlabel)
        r->label = (r->label + 1) % LABELS;
    r->pending = pdu;
    r->plabel = vendor_send(r->label, ctype, pdu, n);
    r->label = (r->label + 1) % LABELS;
    bt_timer(T_AVRCP, CMD_TIMEOUT);
}

/* (Re-)register for the headset's volume changes */
static void register_volume(void)
{
    struct avrcp *r = &bt->avrcp;
    uint8_t *o = params();
    o[0] = EVENT_VOLUME;
    put_be32(o + 1, 0);             /* playback interval: unused */
    vendor_send(r->nlabel, CTYPE_NOTIFY, PDU_REGISTER, 5);
}

/* Send the Rockbox volume if the headset has another */
static void send_volume(void)
{
    struct avrcp *r = &bt->avrcp;
    if (r->abs < ABS_ARMED || r->pending || !vol_known)
        return;
    /* Only while the stream is routed here: otherwise vol_cb is the
     * built-in output's (the apps keep one volume per output) */
    if (pcm_current_sink() != PCM_SINK_BT)
        return;
    int v = to_abs(vol_cb);
    if (r->abs == ABS_ARMED)
        v = MIN(v, r->vol);             /* the first: no louder than now */
    else if (v == r->vol)
        return;
    params()[0] = r->sent = v;
    command(CTYPE_CONTROL, PDU_SET_VOLUME, 1);
}

/* Tell the apps the headset's volume (also how the PCM volume is applied
 * again after a change of r->abs) */
static void volume_to_apps(void)
{
    if (vol_known)
        button_queue_post(SYS_BT_VOLUME, from_abs(bt->avrcp.vol));
}

/* Absolute volume off: the PCM carries the volume again */
static void abs_off(void)
{
    struct avrcp *r = &bt->avrcp;
    bool was_on = r->abs == ABS_ON;
    r->abs = ABS_NONE;
    r->pending = 0;
    if (was_on && vol_known)
        button_queue_post(SYS_BT_VOLUME, vol_cb);
}

/* A response to our command or to the volume notification */
static void controller_rx(const uint8_t *p, size_t len)
{
    struct avrcp *r = &bt->avrcp;
    uint8_t label = p[0] >> AVCTP_LABEL_SHIFT;
    if (len < VD_PARAMS || get_be16(p + 1) != AVRCP_PID ||
        p[AVC_OPCODE] != OP_VENDOR)
        return;
    uint8_t rsp = p[AVC_CTYPE] & AVC_CTYPE_MASK, pdu = p[VD_PDU];
    const uint8_t *a = p + VD_PARAMS;
    size_t n = MIN(get_be16(p + VD_PDU + 2), len - VD_PARAMS);

    if (pdu == PDU_REGISTER && label == r->nlabel && r->abs != ABS_NONE) {
        if ((rsp != RSP_INTERIM && rsp != RSP_CHANGED) || n < 2 ||
            a[0] != EVENT_VOLUME) {
            logf("bt: avrcp volume notification refused (%02x)", rsp);
            abs_off();
            return;
        }
        r->vol = a[1] & VOLUME_MAX;
        logf("bt: avrcp headset volume %u%s", r->vol,
             rsp == RSP_CHANGED ? " (changed)" : "");
        if (rsp == RSP_CHANGED) {
            register_volume();
            if (r->abs == ABS_ON)
                volume_to_apps();
        } else if (r->abs == ABS_ASKED) {
            r->abs = ABS_ARMED;
            r->pending = 0;
            bt_timer(T_AVRCP, 0);
            send_volume();
        }
        return;
    }
    if (!r->pending || pdu != r->pending || label != r->plabel)
        return;
    r->pending = 0;
    bt_timer(T_AVRCP, 0);
    switch (pdu) {
    case PDU_GET_CAPS:
        if (rsp == RSP_STABLE && n >= 2 && a[0] == CAP_EVENTS &&
            memchr(a + 2, EVENT_VOLUME, MIN((size_t)a[1], n - 2))) {
            r->pending = PDU_REGISTER;      /* until the interim response */
            r->retries = 0;
            register_volume();
            bt_timer(T_AVRCP, CMD_TIMEOUT);
        } else {
            logf("bt: avrcp: no absolute volume");
            r->abs = ABS_NONE;
        }
        break;
    case PDU_SET_VOLUME:
        if (rsp != RSP_ACCEPTED || n < 1) {
            logf("bt: avrcp volume refused (%02x)", rsp);
            abs_off();
            break;
        }
        r->vol = a[0] & VOLUME_MAX;
        if (r->abs != ABS_ON || r->vol != r->sent) {
            /* first one taken (the PCM goes to full scale), or the
             * headset chose another volume: Rockbox takes it */
            r->abs = ABS_ON;
            vol_cb = from_abs(r->vol);
            volume_to_apps();
        }
        send_volume();                  /* changed meanwhile */
        break;
    }
}

/* One AVCTP message (fragments: ignored) */
static void avctp_rx(struct l2cap_chan *c, const uint8_t *p, size_t len)
{
    if (len < AVCTP_HDR || (p[0] & AVCTP_PKT_TYPE_MASK))
        return;
    if (p[0] & AVCTP_RSP)
        controller_rx(p, len);
    else
        target_rx(c, p, len);
}

/* The channel is up: does the headset have absolute volume? */
static void controller_start(void)
{
    struct avrcp *r = &bt->avrcp;
    memset(r, 0, sizeof *r);
    r->nlabel = LABELS - 1;
    r->abs = ABS_ASKED;
    params()[0] = CAP_EVENTS;
    command(CTYPE_STATUS, PDU_GET_CAPS, 1);
}

/* Channel events: the first AVCTP channel is ours, others are closed */
void avrcp_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len)
{
    switch (ev) {
    case CHAN_OPEN:                 /* the headset's, or ours (avrcp_timer) */
        if (!bt->avctp || c == bt->avctp) {
            bt->avctp = c;
            controller_start();
        } else {
            l2cap_close(c);
        }
        break;
    case CHAN_DATA:
        if (c == bt->avctp)
            avctp_rx(c, p, len);
        break;
    case CHAN_CLOSED:
        if (c == bt->avctp) {
            bt->avctp = NULL;
            abs_off();
            bt->avrcp.play_reg = false;
            bt_timer(T_AVRCP, 0);
            bt_timer(T_PLAY, 0);
        }
        break;
    }
}

/* Our command again, under a new label (the volume notification keeps
 * its own) */
static void resend(void)
{
    struct avrcp *r = &bt->avrcp;
    switch (r->pending) {
    case PDU_GET_CAPS:
        params()[0] = CAP_EVENTS;
        command(CTYPE_STATUS, PDU_GET_CAPS, 1);
        break;
    case PDU_REGISTER:
        register_volume();
        bt_timer(T_AVRCP, CMD_TIMEOUT);
        break;
    case PDU_SET_VOLUME:
        params()[0] = r->sent;
        command(CTYPE_CONTROL, PDU_SET_VOLUME, 1);
        break;
    }
}

/* The headset did not connect AVRCP after the stream opened: we do. Or
 * our command went unanswered: again, then no absolute volume. */
void avrcp_timer(void)
{
    struct avrcp *r = &bt->avrcp;
    if (bt->avctp) {
        if (r->pending && r->retries < CMD_RETRIES) {
            logf("bt: avrcp command %02x timed out: again", r->pending);
            r->retries++;
            resend();
        } else if (r->pending) {
            logf("bt: avrcp command %02x timed out", r->pending);
            abs_off();
        }
    } else if (bt->av.state >= ST_OPEN) {
        bt->avctp = l2cap_open(PSM_AVCTP);
    }
}

/* T_PLAY: tell the headset once the play status changes; it registers
 * again for the next change */
void avrcp_play_timer(void)
{
    struct avrcp *r = &bt->avrcp;
    if (!r->play_reg || !bt->avctp)
        return;
    uint8_t st = play_status();
    if (st == r->play) {
        bt_timer(T_PLAY, PLAY_POLL);
        return;
    }
    logf("bt: avrcp play status %u", st);
    r->play_reg = false;
    params()[0] = EVENT_PLAYBACK;
    params()[1] = r->play = st;
    vendor_send(r->play_label, RSP_CHANGED, PDU_REGISTER, 2);
}

/* BT_MSG_VOLUME: the Rockbox volume changed */
void avrcp_volume(void)
{
    bt->avrcp.vol_posted = false;
    send_volume();
}

/* Any thread (firmware/sound.c): the Rockbox volume, cb within [min_cb,
 * max_cb]; true if the headset plays at it itself */
bool bt_volume(int cb, int min_cb, int max_cb)
{
    vol_cb = cb;
    vol_min = min_cb;
    vol_max = max_cb;
    vol_known = true;
    struct bt_state *s = bt;
    if (!s || s->avrcp.abs < ABS_ARMED)
        return false;
    if (!s->avrcp.vol_posted) {
        s->avrcp.vol_posted = true;
        queue_post(&s->queue, BT_MSG_VOLUME, 0);
    }
    return s->avrcp.abs == ABS_ON && pcm_current_sink() == PCM_SINK_BT;
}
