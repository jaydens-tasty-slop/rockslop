/* SPDX-License-Identifier: GPL-2.0-or-later */
/* AVDTP 1.3 signalling and the A2DP 1.3 source stream: one SBC source
 * endpoint, configured by us (discover, get capabilities, set
 * configuration, open, start) or by the sink.
 *
 * The first AVDTP channel on the link carries signalling; the side that
 * sends OPEN then opens the second, the media channel (media.c sends on
 * it). Only single-packet signals are used. We START when the PCM sink
 * plays and SUSPEND after it has been idle for IDLE_SUSPEND.
 *
 * Timers: T_AVDTP delays our discovery when the sink opened signalling
 * (it usually configures us itself), opens signalling when a sink that
 * paged us opened none (the WH-1000XM3 sometimes connects hands-free
 * only), times out our pending command (a query is asked again, anything
 * else drops the link) and restarts a stream the sink suspended while
 * Rockbox plays; T_SUSPEND is the idle suspend. */
#include "bt.h"
#include <string.h>
#include "button.h"
#include "system.h"

/* Signal identifiers */
#define AV_DISCOVER         0x01
#define AV_GET_CAPS         0x02
#define AV_SET_CONFIG       0x03
#define AV_GET_CONFIG       0x04
#define AV_RECONFIGURE      0x05
#define AV_OPEN             0x06
#define AV_START            0x07
#define AV_CLOSE            0x08
#define AV_SUSPEND          0x09
#define AV_ABORT            0x0a
#define AV_SECURITY         0x0b
#define AV_GET_ALL_CAPS     0x0c
#define AV_DELAY_REPORT     0x0d

/* Signalling header, single packet: transaction label << 4 | packet type
 * << 2 | message type, then the signal identifier */
#define AV_HDR              2
#define AV_LABEL_SHIFT      4
#define AV_LABEL_MASK       0x0f
#define AV_PKT_TYPE_MASK    0x0c    /* 0: single packet */
#define AV_MSG_MASK         0x03
#define AV_SIG_MASK         0x3f

/* Message types */
#define MSG_CMD             0
#define MSG_GREJECT         1       /* general reject: unknown signal */
#define MSG_ACCEPT          2
#define MSG_REJECT          3

/* Error codes */
#define E_BAD_ACP_SEID      0x12
#define E_SEP_IN_USE        0x13
#define E_BAD_SERV_CAT      0x17
#define E_NOT_SUPPORTED     0x19
#define E_UNSUPP_CONFIG     0x29
#define E_BAD_STATE         0x31

/* SEIDs take bits 7..2 of their byte */
#define SEID_SHIFT          2
/* Set configuration parameters: ACP SEID, INT SEID, then capabilities */
#define SET_CONFIG_SEIDS    2

/* Stream endpoint information in a discover response: SEID | in use, then
 * media type << 4 | TSEP */
#define SEP_INFO_LEN        2
#define SEP_INFO_IN_USE     0x02
#define SEP_INFO_SRC        0x00
#define SEP_INFO_SNK        0x08
#define MEDIA_TYPE_SHIFT    4
#define MEDIA_AUDIO         0x00
#define CODEC_SBC           0x00

/* Service capabilities: category, LOSC (length of service capability),
 * then LOSC bytes */
#define CAT_HDR             2
#define CAT_TRANSPORT       0x01    /* media transport, LOSC 0 */
#define CAT_CODEC           0x07    /* media codec */
/* Media codec capability: media type << 4, codec type, codec information */
#define CODEC_INFO          (CAT_HDR + 2)       /* offset of the codec info */
#define CODEC_LOSC          (2 + SBC_INFO_LEN)

#define OUR_SEID            1

/* SBC codec information (A2DP 1.3, 4.3.2), 4 bytes:
 *   [0] sampling frequency (bits 7..4) | channel mode (3..0)
 *   [1] block length (7..4) | subbands (3..2) | allocation method (1..0)
 *   [2] minimum bitpool, [3] maximum bitpool
 * Capabilities set every bit they support, a configuration one per field. */
#define SBC_INFO_LEN        4
#define SBC_FREQ_MASK       0xf0
#define SBC_44100           0x20
#define SBC_MODE_MASK       0x0f
#define SBC_STEREO          0x02
#define SBC_JOINT           0x01
#define SBC_BLOCKS_MASK     0xf0
#define SBC_BLOCKS_4        0x80
#define SBC_BLOCKS_8        0x40
#define SBC_BLOCKS_12       0x20
#define SBC_BLOCKS_16       0x10
#define SBC_SUBBANDS_MASK   0x0c
#define SBC_SUBBANDS_4      0x08
#define SBC_SUBBANDS_8      0x04
#define SBC_ALLOC_MASK      0x03
#define SBC_SNR             0x02
#define SBC_LOUDNESS        0x01
/* Our highest bitpool: A2DP's recommended high quality setting for
 * 44.1 kHz joint stereo */
#define SBC_BITPOOL_MAX     53

#define SIG_TIMEOUT         (5 * HZ)    /* our command unanswered: disconnect */
#define SIG_RETRIES         2           /* ...but ask a query this often again;
                                         * also our channel request */
#define SINK_CONFIG_WAIT    (2 * HZ)    /* sink opened signalling: let it act */
#define SINK_OPEN_WAIT      (3 * HZ)    /* sink paged us: open ours if it has not */
#define IDLE_SUSPEND        (3 * HZ)    /* PCM sink idle this long: suspend */
#define AVRCP_WAIT          (2 * HZ)    /* stream open, no AVRCP yet: connect */
#define RESTART_DELAY       HZ          /* sink suspended, we play: START */
#define MAX_RESTARTS        3           /* ...this often per play request */
#define ANNOUNCE_REPEAT     (2 * HZ)    /* audio state unacknowledged: again */

/* Ours: media transport, and SBC at 44.1 kHz stereo or joint stereo with
 * every block length, subband count and allocation method */
static const uint8_t our_caps[] = {
    CAT_TRANSPORT, 0,
    CAT_CODEC, CODEC_LOSC, MEDIA_AUDIO << MEDIA_TYPE_SHIFT, CODEC_SBC,
    SBC_44100 | SBC_STEREO | SBC_JOINT,
    SBC_BLOCKS_MASK | SBC_SUBBANDS_MASK | SBC_ALLOC_MASK,
    BT_SBC_BITPOOL_MIN, SBC_BITPOOL_MAX,
};
/* A configuration (set configuration, get configuration response) is
 * our_caps with the chosen SBC information */
#define CAPS_SBC_INFO       (CAT_HDR + CODEC_INFO)  /* its offset in our_caps */
#define CONFIG_LEN          (CAPS_SBC_INFO + SBC_INFO_LEN)

/* Parameters of the signal we send next */
static uint8_t *out(void)
{
    return bt->tx + BT_HDR + AV_HDR;
}

/* Send a signal with len bytes of parameters from out() */
static void av_send(uint8_t label, uint8_t msg, uint8_t sig, size_t len)
{
    uint8_t *p = bt->tx + BT_HDR;
    p[0] = label << AV_LABEL_SHIFT | msg;
    p[1] = sig;
    if (bt->av.sig)
        l2cap_send(bt->av.sig, len + AV_HDR);
}

/* Send a command; av_rx() matches the response, T_AVDTP times it out */
static void av_cmd(uint8_t sig, size_t len)
{
    struct avdtp *a = &bt->av;
    logf("bt: avdtp cmd %u (state %d)", sig, a->state);
    if (sig != a->pending)      /* a new command, not one asked again */
        a->retries = 0;
    a->pending = sig;
    a->plabel = a->label;
    av_send(a->label, MSG_CMD, sig, len);
    a->label = (a->label + 1) & AV_LABEL_MASK;
    bt_timer(T_AVDTP, SIG_TIMEOUT);
}

/* A command whose only parameter is the sink's SEID */
static void av_cmd_seid(uint8_t sig)
{
    out()[0] = bt->av.rseid << SEID_SHIFT;
    av_cmd(sig, 1);
}

static bool one_bit(unsigned v)
{
    return v && !(v & (v - 1));
}

/* Pick our preferred configuration from a sink's SBC capabilities: joint
 * stereo over stereo, the most blocks, 8 subbands, loudness, and the
 * widest bitpool range within ours. False if nothing fits. */
static bool sbc_choose(const uint8_t *in, uint8_t *cfg)
{
    static const uint8_t blocks[] = {
        SBC_BLOCKS_16, SBC_BLOCKS_12, SBC_BLOCKS_8, SBC_BLOCKS_4
    };
    if (!(in[0] & SBC_44100) || !(in[0] & (SBC_JOINT | SBC_STEREO)))
        return false;
    cfg[0] = SBC_44100 | (in[0] & SBC_JOINT ? SBC_JOINT : SBC_STEREO);
    cfg[1] = 0;
    for (int i = 0; i < (int)sizeof blocks && !cfg[1]; i++)
        cfg[1] = in[1] & blocks[i];
    cfg[1] |= in[1] & SBC_SUBBANDS_8 ? SBC_SUBBANDS_8 : SBC_SUBBANDS_4;
    cfg[1] |= in[1] & SBC_LOUDNESS ? SBC_LOUDNESS : SBC_SNR;
    cfg[2] = MAX(in[2], BT_SBC_BITPOOL_MIN);
    cfg[3] = MIN(in[3], SBC_BITPOOL_MAX);
    return (cfg[1] & SBC_BLOCKS_MASK) && (in[1] & SBC_SUBBANDS_MASK) &&
           (in[1] & SBC_ALLOC_MASK) && cfg[2] <= cfg[3];
}

/* A configuration from the sink: one choice per field within our caps */
static bool sbc_valid(const uint8_t *c)
{
    return (c[0] & SBC_FREQ_MASK) == SBC_44100 &&
           one_bit(c[0] & SBC_MODE_MASK) &&
           (c[0] & (SBC_JOINT | SBC_STEREO)) &&
           one_bit(c[1] & SBC_BLOCKS_MASK) &&
           one_bit(c[1] & SBC_SUBBANDS_MASK) &&
           one_bit(c[1] & SBC_ALLOC_MASK) && c[2] >= BT_SBC_BITPOOL_MIN &&
           c[2] <= c[3] && c[2] <= SBC_BITPOOL_MAX;
}

/* Remember a configuration and open the encoder for it */
static bool configure(const uint8_t *cfg)
{
    struct avdtp *a = &bt->av;
    struct bt_sbc_params *s = &a->sbc;
    memcpy(a->cfg, cfg, SBC_INFO_LEN);
    s->freq = BT_SBC_44100;
    s->mode = cfg[0] & SBC_JOINT ? BT_SBC_JOINT : BT_SBC_STEREO;
    s->blocks = cfg[1] & SBC_BLOCKS_16 ? 16 : cfg[1] & SBC_BLOCKS_12 ? 12 :
                cfg[1] & SBC_BLOCKS_8 ? 8 : 4;
    s->subbands = cfg[1] & SBC_SUBBANDS_8 ? 8 : 4;
    s->alloc = cfg[1] & SBC_LOUDNESS ? BT_SBC_LOUDNESS : BT_SBC_SNR;
    s->bitpool = MIN(cfg[3], SBC_BITPOOL_MAX);
    logf("bt: sbc %u blocks %u subbands mode %u alloc %u bitpool %u",
         s->blocks, s->subbands, s->mode, s->alloc, s->bitpool);
    return media_open();
}

/* Our configuration as service capabilities; returns CONFIG_LEN */
static size_t put_config(uint8_t *o)
{
    memcpy(o, our_caps, CAPS_SBC_INFO);
    memcpy(o + CAPS_SBC_INFO, bt->av.cfg, SBC_INFO_LEN);
    return CONFIG_LEN;
}

/* Back to idle: stop the media path, close the encoder and the media
 * channel, forget any pending command */
static void stream_reset(void)
{
    struct avdtp *a = &bt->av;
    struct l2cap_chan *m = a->media;
    media_stop();
    media_close();
    a->media = NULL;
    a->state = ST_IDLE;
    a->pending = 0;
    a->ncand = 0;
    bt_timer(T_SUSPEND, 0);
    if (m)
        l2cap_close(m);
}

/* START accepted, by either side: send media; suspend again soon if the
 * PCM sink stopped meanwhile */
static void stream_started(void)
{
    bt->av.state = ST_STREAMING;
    media_start();
    if (!bt->av.want)
        bt_timer(T_SUSPEND, IDLE_SUSPEND);
}

static void post_audio_state(void)
{
    button_queue_post(bt->av.announced ? SYS_BT_AUDIO_CONNECTED :
                      SYS_BT_AUDIO_DISCONNECTED, 0);
}

/* Tell the apps (audio routing) once per device. A screen that drops
 * system events loses the event, so it is posted again until the apps
 * acknowledge this state. */
static void audio_device(bool connected)
{
    struct avdtp *a = &bt->av;
    if (a->announced != connected) {
        a->announced = connected;
        post_audio_state();
        bt_timer(T_ANNOUNCE, ANNOUNCE_REPEAT);
    }
}

/* Again only once the button queue is empty: the last one was read (and
 * dropped), and a UI thread that reads nothing does not fill the queue */
void avdtp_announce_timer(void)
{
    struct avdtp *a = &bt->av;
    if (a->acked == a->announced)
        return;
    if (button_queue_empty())
        post_audio_state();
    bt_timer(T_ANNOUNCE, ANNOUNCE_REPEAT);
}

/* UI thread: the apps routed the audio for this event. Naming the state
 * keeps a late event's acknowledgement from ending the next one's. */
void bt_audio_acknowledge(long event)
{
    struct bt_state *s = bt;
    if (s)
        s->av.acked = event == SYS_BT_AUDIO_CONNECTED;
}

/* The media channel is up */
static void stream_opened(void)
{
    struct avdtp *a = &bt->av;
    a->state = ST_OPEN;
    audio_device(true);
    if (!bt->avctp)
        bt_timer(T_AVRCP, AVRCP_WAIT);
    hfp_stream_opened();
    if (a->want && !a->pending)
        av_cmd_seid(AV_START);
}

/* Initiator: discover -> get capabilities -> set configuration -> open */
static void discover(void)
{
    struct avdtp *a = &bt->av;
    if (a->sig && a->sig->state == CH_OPEN && a->state == ST_IDLE && !a->pending)
        av_cmd(AV_DISCOVER, 0);
}

/* Ask the first candidate sink endpoint for its capabilities */
static void next_candidate(void)
{
    struct avdtp *a = &bt->av;
    if (!a->ncand) {
        logf("bt: no usable sink endpoint");
        return;
    }
    a->rseid = a->cand[0];
    av_cmd_seid(AV_GET_CAPS);
}

/* The first candidate does not suit us: try the next */
static void drop_candidate(void)
{
    struct avdtp *a = &bt->av;
    if (a->ncand)
        memmove(a->cand, a->cand + 1, --a->ncand);
    next_candidate();
}

/* Get capabilities response: configure the candidate if it takes SBC that
 * we can send, otherwise move on */
static void got_caps(const uint8_t *p, size_t len)
{
    uint8_t cfg[SBC_INFO_LEN];
    for (size_t i = 0; i + CAT_HDR <= len && i + CAT_HDR + p[i + 1] <= len;
         i += CAT_HDR + p[i + 1]) {
        const uint8_t *c = p + i;
        if (c[0] == CAT_CODEC && c[1] >= CODEC_LOSC &&
            (c[2] >> MEDIA_TYPE_SHIFT) == MEDIA_AUDIO && c[3] == CODEC_SBC &&
            sbc_choose(c + CODEC_INFO, cfg) && configure(cfg)) {
            uint8_t *o = out();
            o[0] = bt->av.rseid << SEID_SHIFT;      /* ACP SEID */
            o[1] = OUR_SEID << SEID_SHIFT;          /* INT SEID */
            put_config(o + SET_CONFIG_SEIDS);
            av_cmd(AV_SET_CONFIG, SET_CONFIG_SEIDS + CONFIG_LEN);
            return;
        }
    }
    drop_candidate();
}

/* Initiator: the response to our pending command sig */
static void av_response(uint8_t sig, uint8_t msg, const uint8_t *p, size_t len)
{
    struct avdtp *a = &bt->av;
    if (msg != MSG_ACCEPT) {
        logf("bt: avdtp signal %u rejected", sig);
        if (sig == AV_GET_CAPS)
            drop_candidate();
        else if (sig == AV_SET_CONFIG && a->state == ST_IDLE) {
            media_close();
            drop_candidate();
        }
        return;
    }
    switch (sig) {
    case AV_DISCOVER:
        a->ncand = 0;
        for (size_t i = 0; i + 1 < len && a->ncand < sizeof a->cand;
             i += SEP_INFO_LEN)
            if (!(p[i] & SEP_INFO_IN_USE) &&
                (p[i + 1] >> MEDIA_TYPE_SHIFT) == MEDIA_AUDIO &&
                (p[i + 1] & SEP_INFO_SNK))
                a->cand[a->ncand++] = p[i] >> SEID_SHIFT; /* free audio sink */
        next_candidate();
        break;
    case AV_GET_CAPS:
        if (a->state == ST_IDLE)
            got_caps(p, len);
        break;
    case AV_SET_CONFIG:
        if (a->state == ST_IDLE) {
            a->state = ST_CONFIGURED;
            av_cmd_seid(AV_OPEN);
        }
        break;
    case AV_OPEN:
        if (a->state == ST_CONFIGURED) {
            a->state = ST_OPENING;      /* we open the media channel */
            if (!(a->media = l2cap_open(PSM_AVDTP)))
                stream_reset();
        }
        break;
    case AV_START:
        if (a->state == ST_OPEN)
            stream_started();
        break;
    case AV_SUSPEND:
        if (a->state == ST_STREAMING) {
            a->state = ST_OPEN;
            media_stop();
            if (a->want)
                av_cmd_seid(AV_START);
        }
        break;
    case AV_CLOSE:
    case AV_ABORT:
        stream_reset();
        break;
    }
}

/* Acceptor: set configuration from the sink with INT SEID int_seid and
 * its service capabilities. Returns 0 or the error code, with the
 * offending service category in cat. */
static uint8_t set_config(uint8_t int_seid, const uint8_t *p, size_t len,
                          uint8_t *cat)
{
    const uint8_t *cfg = NULL;
    for (size_t i = 0; i + CAT_HDR <= len && i + CAT_HDR + p[i + 1] <= len;
         i += CAT_HDR + p[i + 1]) {
        const uint8_t *c = p + i;
        *cat = c[0];
        if (c[0] == CAT_CODEC) {
            if (c[1] < CODEC_LOSC ||
                (c[2] >> MEDIA_TYPE_SHIFT) != MEDIA_AUDIO ||
                c[3] != CODEC_SBC || !sbc_valid(c + CODEC_INFO))
                return E_UNSUPP_CONFIG;
            cfg = c + CODEC_INFO;
        } else if (c[0] != CAT_TRANSPORT) {
            return E_BAD_SERV_CAT;
        }
    }
    *cat = CAT_CODEC;
    if (!cfg || !configure(cfg))
        return E_UNSUPP_CONFIG;
    struct avdtp *a = &bt->av;
    a->rseid = int_seid;
    a->state = ST_CONFIGURED;
    a->pending = 0;         /* our own discovery, if any, is moot */
    bt_timer(T_AVDTP, 0);
    return 0;
}

/* Acceptor: answer a command from the sink, then act on it */
static void av_command(uint8_t label, uint8_t sig, const uint8_t *p, size_t len)
{
    struct avdtp *a = &bt->av;
    uint8_t *o = out();
    uint8_t seid = len ? p[0] >> SEID_SHIFT : 0, err = 0, cat = 0;
    size_t n = 0;

    /* signals up to GET_ALL_CAPS, but DISCOVER, start with our SEID */
    if (sig != AV_DISCOVER && sig != AV_DELAY_REPORT && seid != OUR_SEID &&
        sig <= AV_GET_ALL_CAPS) {
        err = E_BAD_ACP_SEID;
    } else switch (sig) {
    case AV_DISCOVER:
        o[0] = OUR_SEID << SEID_SHIFT |
               (a->state != ST_IDLE ? SEP_INFO_IN_USE : 0);
        o[1] = MEDIA_AUDIO << MEDIA_TYPE_SHIFT | SEP_INFO_SRC;
        n = SEP_INFO_LEN;
        break;
    case AV_GET_CAPS:
    case AV_GET_ALL_CAPS:
        memcpy(o, our_caps, sizeof our_caps);
        n = sizeof our_caps;
        break;
    case AV_SET_CONFIG:
        if (a->state != ST_IDLE)
            err = E_SEP_IN_USE;
        else if (len < SET_CONFIG_SEIDS)
            err = E_UNSUPP_CONFIG;
        else
            err = set_config(p[1] >> SEID_SHIFT, p + SET_CONFIG_SEIDS,
                             len - SET_CONFIG_SEIDS, &cat);
        break;
    case AV_GET_CONFIG:
        if (a->state == ST_IDLE)
            err = E_BAD_STATE;
        else
            n = put_config(o);
        break;
    case AV_RECONFIGURE:
        err = E_SEP_IN_USE;
        cat = CAT_CODEC;
        break;
    case AV_OPEN:
        if (a->state != ST_CONFIGURED)
            err = E_BAD_STATE;
        else
            a->state = ST_OPENING;      /* the sink opens the media channel */
        break;
    case AV_START:
        if (a->state != ST_OPEN)
            err = E_BAD_STATE;
        break;
    case AV_SUSPEND:
        if (a->state != ST_STREAMING)
            err = E_BAD_STATE;
        break;
    case AV_CLOSE:
        if (a->state < ST_OPEN)
            err = E_BAD_STATE;
        break;
    case AV_ABORT:
    case AV_DELAY_REPORT:
        break;
    case AV_SECURITY:
        err = E_NOT_SUPPORTED;
        break;
    default:
        av_send(label, MSG_GREJECT, sig, 0);
        return;
    }

    if (err) {
        /* the error code, after the service category or the SEID where
         * the reject has one */
        logf("bt: avdtp signal %u: error %02x", sig, err);
        if (sig == AV_SET_CONFIG || sig == AV_RECONFIGURE) {
            o[0] = cat;
            o[1] = err;
            n = 2;
        } else if (sig == AV_START || sig == AV_SUSPEND) {
            o[0] = seid << SEID_SHIFT;
            o[1] = err;
            n = 2;
        } else {
            o[0] = err;
            n = 1;
        }
        av_send(label, MSG_REJECT, sig, n);
        return;
    }
    av_send(label, MSG_ACCEPT, sig, n);

    /* after the accept, so the sink hears it before any media */
    switch (sig) {
    case AV_START:
        stream_started();
        break;
    case AV_SUSPEND:
        a->state = ST_OPEN;
        media_stop();
        /* Rockbox still plays: start again, a few times at most */
        if (a->want && a->restarts < MAX_RESTARTS) {
            a->restarts++;
            bt_timer(T_AVDTP, RESTART_DELAY);
        }
        break;
    case AV_CLOSE:
    case AV_ABORT:
        stream_reset();
        break;
    }
}

/* One signal on the signalling channel */
static void av_rx(const uint8_t *p, size_t len)
{
    struct avdtp *a = &bt->av;
    if (len < AV_HDR || (p[0] & AV_PKT_TYPE_MASK))  /* fragmented: not used */
        return;
    uint8_t label = p[0] >> AV_LABEL_SHIFT, msg = p[0] & AV_MSG_MASK,
            sig = p[1] & AV_SIG_MASK;
    logf("bt: avdtp rx %u msg %u (state %d, pending %u)", sig, msg, a->state,
         a->pending);
    if (msg == MSG_CMD) {
        av_command(label, sig, p + AV_HDR, len - AV_HDR);
    } else if (a->pending && sig == a->pending && label == a->plabel) {
        a->pending = 0;
        bt_timer(T_AVDTP, 0);
        av_response(sig, msg, p + AV_HDR, len - AV_HDR);
    }
}

void avdtp_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len)
{
    struct avdtp *a = &bt->av;
    switch (ev) {
    case CHAN_OPEN:
        if (c == a->sig) {              /* ours: configure the sink */
            discover();
        } else if (!a->sig) {           /* the sink's: give it time first */
            a->sig = c;
            bt_timer(T_AVDTP, SINK_CONFIG_WAIT);
        } else if (a->state == ST_OPENING && (!a->media || c == a->media)) {
            a->media = c;
            stream_opened();
        } else {
            l2cap_close(c);
        }
        break;
    case CHAN_DATA:
        if (c == a->sig)
            av_rx(p, len);
        break;
    case CHAN_CLOSED:
        if (c == a->sig) {
            a->sig = NULL;
            stream_reset();
            bt_timer(T_AVDTP, 0);
            audio_device(false);
        } else if (c == a->media) {
            a->media = NULL;
            stream_reset();
        }
        break;
    }
}

/* Whether the sink may open an AVDTP channel now: signalling if there is
 * none yet, media while we wait for it after OPEN */
bool avdtp_accept(void)
{
    struct avdtp *a = &bt->av;
    return !a->sig || (a->state == ST_OPENING && !a->media);
}

/* Ask for our signalling channel; SIG_TIMEOUT for the sink to answer */
static void open_sig(void)
{
    bt->av.sig = l2cap_open(PSM_AVDTP);
    bt_timer(T_AVDTP, SIG_TIMEOUT);
}

/* The link is secure: open signalling if we paged, else give the sink
 * SINK_OPEN_WAIT to open it */
void avdtp_link_secured(void)
{
    if (bt->av.sig)
        return;
    bt->av.retries = 0;
    if (bt->link.flags & LF_OUTGOING)
        open_sig();
    else
        bt_timer(T_AVDTP, SINK_OPEN_WAIT);
}

void avdtp_timer(int t)
{
    struct avdtp *a = &bt->av;
    if (t == T_SUSPEND) {
        if (!a->want && a->state == ST_STREAMING && !a->pending)
            av_cmd_seid(AV_SUSPEND);
    } else if ((a->pending == AV_DISCOVER || a->pending == AV_GET_CAPS) &&
               a->retries < SIG_RETRIES) {
        /* a query changes nothing: ask again (at the range edge a DISCOVER
         * went unanswered on a link that was still up); a late answer to
         * the first has the old label and is ignored */
        logf("bt: avdtp signal %u timed out: again", a->pending);
        a->retries++;
        if (a->pending == AV_DISCOVER)
            av_cmd(AV_DISCOVER, 0);
        else
            av_cmd_seid(AV_GET_CAPS);
    } else if (a->pending) {
        logf("bt: avdtp signal %u timed out", a->pending);
        hci_drop_silent();
    } else if (!a->sig) {
        if (bt->link.state == LINK_UP) {
            logf("bt: the sink opened no signalling: opening ours");
            open_sig();
        }
    } else if (a->sig->state != CH_OPEN) {
        /* a WH-1000XM3 that had just paged us never answered one, and
         * would get no stream on a link that stayed up: a new request
         * (new CID), then drop the link */
        logf("bt: avdtp channel unanswered");
        l2cap_close(a->sig);
        if (a->retries++ < SIG_RETRIES)
            open_sig();
        else
            hci_drop_silent();
    } else if (a->state == ST_OPEN && a->want) {
        av_cmd_seid(AV_START);
    } else {
        discover();
    }
}

/* Power-off: close the stream first, as a headset may keep a dropped one.
 * True if a CLOSE is on its way (the stream goes idle on the response). */
bool avdtp_close(void)
{
    struct avdtp *a = &bt->av;
    if ((a->state != ST_OPEN && a->state != ST_STREAMING) || a->pending)
        return false;
    media_stop();
    av_cmd_seid(AV_CLOSE);
    return true;
}

/* The PCM sink started (play) or stopped playing */
void avdtp_stream_want(bool play)
{
    struct avdtp *a = &bt->av;
    logf("bt: stream want %d (state %d, pending %u)", play, a->state,
         a->pending);
    a->want = play;
    a->restarts = 0;
    if (!play) {
        if (a->state == ST_STREAMING)
            bt_timer(T_SUSPEND, IDLE_SUSPEND);
    } else {
        bt_timer(T_SUSPEND, 0);
        if (a->state == ST_OPEN && !a->pending)
            av_cmd_seid(AV_START);
    }
}
