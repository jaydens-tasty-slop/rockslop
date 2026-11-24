/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PCM_SINK_BT and the A2DP media path. While the stream is started, a
 * tick task pulls PCM from the mixer (as a DMA driver would), encodes SBC
 * and sends RTP media packets sized to the channel MTU, paced by elapsed
 * time with a small lead and by ACL credits. It runs in interrupt context
 * so UI work cannot starve the headset; a packet may take a few ticks to
 * encode, so that no tick runs into the next. The SBC bitpool steps down while
 * the link cannot keep up and back up once it has for a while.
 *
 * avdtp.c drives it: media_open() when a configuration is set,
 * media_start()/media_stop() with the stream, media_close() when idle. */
#include "bt.h"
#include <string.h>
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"
#include "pcm_sink.h"
#include "system.h"

#define RATE        BT_MEDIA_RATE
/* Ahead of real time: the headset's buffer then covers 100 ms of stalls
 * on top of the PCM ring (which is smaller while Bluetooth is on) */
#define LEAD        (RATE / 10)
#define MAX_BEHIND  (RATE / 4)      /* further behind: restart the clock */
/* Media packet: RTP header, SBC payload header, SBC frames */
#define RTP_HDR     12
#define RTP_V2      0x80            /* version 2, no padding, extension, CSRC */
#define RTP_PT      96              /* dynamic payload type, marker 0 */
#define RTP_SSRC    1
#define SBC_HDR     1               /* not fragmented | number of frames */
/* SBC frames per media packet at most (the count has 4 bits): 5, as stock
 * sends, one EDR 2-DH5 baseband packet up to bitpool 53; at the range edge
 * they streamed with fewer clock restarts than 10-14 */
#define SBC_MAX_FRAMES 5u
#define MEDIA_HDR   (RTP_HDR + SBC_HDR)
#define PCM_FRAME   4               /* bytes: one 16-bit stereo sample */
/* SBC frames encoded per tick at most, so the tick interrupt ends before
 * the next tick is due (a later one would be lost): ~9 ms with the YP3 ROM
 * encoder, 1.7x the frames real time needs */
#define TICK_FRAMES 6
/* With more ACL buffers than this, one is kept free for signalling */
#define FEW_ACL_BUFS 2
/* Bitpool adaptation: start at BP_START, step down while the controller
 * cannot get packets out in time (a basic rate link to some headsets
 * carries less than bitpool 53, ~330 kbit/s; the XM3 not even at a desk),
 * step back up while the link keeps up. Small steps: big ones dropped
 * 35 -> 18 in 3 s at the edge of range and took ~100 s to come back, and
 * sounded worse there than steps of 4 (XM3 walks 10-23). */
#define BP_START    35              /* stock's highest bitpool */
#define BP_DOWN     4
#define BP_UP       4
#define BP_FLOOR    24              /* lowest bitpool we step down to */
#define BEHIND_SLOW (RATE / 10)     /* this far behind: lower the bitpool */
#define BEHIND_OK   (RATE / 50)     /* within this: the link keeps up */
#define BP_HOLD     (3 * HZ)        /* at least this long between steps down */
#define CALM_TICKS  (5 * HZ)        /* kept up this long: raise it again */
#define CALM_MAX    (60 * HZ)       /* the wait doubles, to this, after each
                                     * step up the link could not carry */
/* At most this many ACL buffers (and under half) in use for CALM_TICKS:
 * the link has room (back in range), so it steps up after CALM_TICKS
 * whatever the doubled wait. Without this, bitpool 24-28 held ~2 min at
 * the desk after a spell at the range edge, where the buffers run low. */
#define ROOM_BUSY   2
#define STATS_TICKS (10 * HZ)       /* BT_MEDIA_STATS summary period */

/* What the mixer gave us; kept outside the stack's allocation because
 * the sink may be selected while Bluetooth is off */
static const uint8_t *pcm_ptr;
static size_t pcm_left;
static volatile bool pcm_on;
static volatile int pcm_locked;
static bool sbc_ready;

/* One frame of PCM into the staging buffer; silence once the mixer stops */
static void pcm_pull(struct bt_media *m)
{
    uint8_t *dst = (uint8_t *)m->pcm_stage;
    size_t want = m->frame_samples * PCM_FRAME;
    while (want) {
        if (!pcm_left) {
            const void *addr;
            size_t size;
            if (!pcm_on ||
                !pcm_play_dma_complete_callback(PCM_DMAST_OK, &addr, &size) ||
                !size) {
                memset(dst, 0, want);
                return;
            }
            pcm_ptr = addr;
            pcm_left = size;
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);
        }
        size_t n = MIN(want, pcm_left);
        memcpy(dst, pcm_ptr, n);
        pcm_ptr += n;
        pcm_left -= n;
        dst += n;
        want -= n;
    }
}

/* Encode up to n more frames into the media packet being built; returns
 * how many */
static int encode_frames(struct bt_media *m, int n)
{
    uint8_t *f = m->buf + BT_HDR + MEDIA_HDR + m->nframes * m->frame_len;
    int done = 0;
    for (; done < n && m->nframes < m->frames && pcm_on; done++) {
        pcm_pull(m);
        if (!bt_sbc_encode(m->pcm_stage, f))
            break;
        f += m->frame_len;
        m->nframes++;
    }
    return done;
}

/* Send the frames encoded so far as one media packet; false (the frames
 * kept for the next try) if the driver had no room */
static bool send_packet(struct bt_media *m)
{
    uint8_t *p = m->buf + BT_HDR;
    int n = m->nframes;
    p[0] = RTP_V2;
    p[1] = RTP_PT;
    put_be16(p + 2, m->seq);        /* sequence number */
    put_be32(p + 4, m->ts);         /* timestamp, in samples */
    put_be32(p + 8, RTP_SSRC);
    p[RTP_HDR] = n;                 /* SBC frames, not fragmented */
    if (!acl_send(m->buf, m->cid, MEDIA_HDR + n * m->frame_len, true))
        return false;
    m->packets++;
    m->seq++;
    m->ts += n * m->frame_samples;
    m->sent += n * m->frame_samples;
    m->nframes = 0;
    return true;
}

/* Whether the controller has buffers for an L2CAP payload of len bytes,
 * besides the one kept for signalling */
static bool credit_for(size_t len)
{
    unsigned frags = (len + L2CAP_HDR + bt->acl_mtu - 1) / bt->acl_mtu;
    return bt->acl_credits >= frags + (bt->acl_total > FEW_ACL_BUFS);
}

/* SBC frames per media packet at the current bitpool */
static void media_size(struct bt_media *m)
{
    m->frame_len = bt_sbc_frame_bytes();
    m->frames = MIN(SBC_MAX_FRAMES,
                    (m->mtu - (unsigned)MEDIA_HDR) / m->frame_len);
}

/* Change the bitpool, within the configured range, between packets. A step
 * up whose frame would not fit a media packet (a small MTU) lowers the top
 * of the range instead. */
static void media_bitpool(struct bt_media *m, int bitpool)
{
    bitpool = MAX(MIN(bitpool, m->bp_max), m->bp_min);
    if (bitpool == m->bitpool || m->nframes || !bt_sbc_set_bitpool(bitpool))
        return;
    if (bt_sbc_frame_bytes() > m->mtu - (unsigned)MEDIA_HDR) {
        bt_sbc_set_bitpool(m->bitpool);
        m->bp_max = bitpool - 1;
        return;
    }
    bool up = bitpool > m->bitpool;
    if (m->bp_rose)                 /* did the last step up hold? */
        m->calm_wait = up ? CALM_TICKS : MIN(2 * m->calm_wait, CALM_MAX);
    m->bp_rose = up;
    m->bitpool = bitpool;
    m->bp_tick = current_tick;
    media_size(m);
    logf("bt: bitpool %d, %u frames of %u bytes per packet", bitpool,
         m->frames, m->frame_len);
}

#ifdef BT_MEDIA_STATS
/* Start a summary period */
static void media_stats_start(struct bt_media *m, long now)
{
    m->st_tick = now;
    m->st_behind = 0;
    m->st_packets = m->packets;
    m->st_waits = m->waits;
    m->st_restarts = m->restarts;
    m->st_credits = bt->acl_total;
}

/* One line per STATS_TICKS: packets sent, bitpool, the worst lag, ticks
 * that waited for ACL buffers, clock restarts, fewest free buffers */
static void media_stats(struct bt_media *m, long now, int32_t behind)
{
    m->st_behind = MAX(m->st_behind, behind);
    m->st_credits = MIN(m->st_credits, bt->acl_credits);
    if (now - m->st_tick < STATS_TICKS)
        return;
    logf("bt: media %u pkts bp %u behind %ld ms waits %u restarts %u "
         "bufs %u/%u", (unsigned)(m->packets - m->st_packets), m->bitpool,
         (long)m->st_behind * 1000 / RATE, (unsigned)(m->waits - m->st_waits),
         (unsigned)(m->restarts - m->st_restarts), m->st_credits,
         bt->acl_total);
    media_stats_start(m, now);
}
#endif

/* Tick task: encode and send what real time plus LEAD asks for, then
 * adapt the bitpool to how far behind that left us. Once the mixer stops,
 * the frames encoded so far still go out. */
static void media_tick(void)
{
    struct bt_media *m = &bt->media;
    if (!m->running || pcm_locked || bt->tx_busy)
        return;
    size_t len = MEDIA_HDR + m->frames * m->frame_len;
    if (!pcm_on) {
        if (m->nframes && credit_for(len))
            send_packet(m);
        return;
    }

    /* m->sent counts the samples sent since tick m->t0; both move on a
     * second at a time */
    long now = current_tick;
    while (now - m->t0 >= HZ) {
        m->t0 += HZ;
        m->sent -= RATE;
    }
    int32_t due = (now - m->t0) * RATE / HZ + LEAD;
    if (due - m->sent > MAX_BEHIND) {
#ifdef BT_MEDIA_STATS
        m->st_behind = MAX(m->st_behind, due - m->sent);
#endif
        m->restarts++;
        m->t0 = now;
        m->sent = 0;
        due = LEAD;
    }
    /* encode frames only as they fall due, so the mixer is drained at the
     * pace of real time, ~3-4 frames a tick (a live source such as FM
     * capture keeps little ahead: a packet's worth at once ran it dry);
     * start a packet only while the controller has room for it (so a
     * starved link leaves none pending and the bitpool can step down);
     * send it once full */
    bool waited = false;
    int budget = TICK_FRAMES;
    while (pcm_on) {
        if (m->nframes == m->frames &&
            (!credit_for(len) || !send_packet(m))) {
            waited = true;
            break;
        }
        int32_t owed = due - m->sent - m->nframes * m->frame_samples;
        int n = MIN(budget, (owed + m->frame_samples - 1) / m->frame_samples);
        if (n <= 0)
            break;
        if (!m->nframes && !credit_for(len)) {
            waited = true;
            break;
        }
        budget -= encode_frames(m, n);
        if (m->nframes < m->frames)
            break;              /* nothing more due, budget spent or no encoder */
    }

    /* frames encoded but not yet sent count as sent */
    int32_t behind = due - m->sent - m->nframes * m->frame_samples;
    m->behind = behind;
    m->waits += waited;
#ifdef BT_MEDIA_STATS
    media_stats(m, now, behind);
#endif
    if (behind > BEHIND_OK)
        m->calm = now;
    if (bt->acl_credits + ROOM_BUSY < bt->acl_total ||
        bt->acl_credits <= bt->acl_total / 2)
        m->busy = now;
    long wait = now - m->busy >= CALM_TICKS ? CALM_TICKS : m->calm_wait;
    if (behind > BEHIND_SLOW && now - m->bp_tick >= BP_HOLD)
        media_bitpool(m, m->bitpool - BP_DOWN);
    else if (now - m->calm >= wait && now - m->bp_tick >= wait)
        media_bitpool(m, m->bitpool + BP_UP);
}

/* Open the encoder for the configuration in bt->av. The bitpool starts at
 * BP_START and moves between the configured minimum (but not below
 * BP_FLOOR) and maximum. */
bool media_open(void)
{
    struct bt_media *m = &bt->media;
    media_close();
    if (!bt_sbc_open(&bt->av.sbc))
        return false;
    sbc_ready = true;
    m->frame_samples = bt_sbc_frame_samples();
    m->bitpool = m->bp_max = bt->av.sbc.bitpool;
    /* cfg[2]: the minimum bitpool of the configuration */
    m->bp_min = MIN(MAX(bt->av.cfg[2], BP_FLOOR), m->bp_max);
    unsigned start = MAX(MIN(BP_START, m->bp_max), m->bp_min);
    if (bt_sbc_set_bitpool(start))
        m->bitpool = start;
    m->frame_len = bt_sbc_frame_bytes();
    m->bp_rose = false;
    m->calm_wait = CALM_TICKS;
    return true;
}

void media_close(void)
{
    if (sbc_ready) {
        media_stop();
        bt_sbc_close();
        sbc_ready = false;
    }
}

/* The stream has started: size packets to the media channel and go */
void media_start(void)
{
    struct bt_media *m = &bt->media;
    struct l2cap_chan *c = bt->av.media;
    if (!c || !sbc_ready)
        return;
    /* fit the controller's buffers, leaving one for signalling */
    unsigned bufs = bt->acl_total > FEW_ACL_BUFS ? bt->acl_total - 1u : 1u;
    size_t mtu = MIN((size_t)c->rmtu, BT_MEDIA_SIZE - BT_HDR);
    mtu = MIN(mtu, bt->acl_mtu * bufs - L2CAP_HDR);
    m->mtu = mtu;
    m->frames = m->nframes = 0;
    if (mtu > MEDIA_HDR) {
        media_size(m);
        /* a small MTU: the highest bitpool in range whose frame fits */
        while (!m->frames && m->bitpool > m->bp_min &&
               bt_sbc_set_bitpool(m->bitpool - 1)) {
            m->bitpool--;
            m->bp_max = m->bitpool;
            media_size(m);
        }
    }
    if (!m->frames) {
        logf("bt: no SBC frame fits a media packet (MTU %u)", (unsigned)mtu);
        return;
    }
    m->cid = c->rcid;
    m->t0 = m->bp_tick = m->calm = m->busy = current_tick;
    m->sent = 0;
    m->behind = 0;
#ifdef BT_MEDIA_STATS
    media_stats_start(m, current_tick);
#endif
    logf("bt: streaming, %u frames of %u bytes per packet, bitpool %u",
         m->frames, m->frame_len, m->bitpool);
    m->running = true;
    /* SBC encoding in the tick needs the full clock, like a codec */
    cpu_boost(true);
    tick_add_task(media_tick);
}

void media_stop(void)
{
    if (bt->media.running) {
        tick_remove_task(media_tick);
        bt->media.running = false;
        cpu_boost(false);
    }
}

/* PCM sink: the mixer's play/stop/lock calls only update the statics
 * above (and wake the thread); the tick task pulls the data */
static void sink_nop(void)
{
}

static void sink_set_freq(uint16_t freq)
{
    (void)freq;                     /* 44.1 kHz only */
}

static void sink_lock(void)
{
    /* with interrupts off so that, in the simulator, a media tick running
     * in parallel finishes first */
    int irq = disable_irq_save();
    pcm_locked++;
    restore_irq(irq);
}

static void sink_unlock(void)
{
    int irq = disable_irq_save();
    pcm_locked--;
    restore_irq(irq);
}

static void sink_play(const void *addr, size_t size)
{
    int irq = disable_irq_save();
    pcm_ptr = addr;
    pcm_left = size;
    pcm_on = true;
    if (bt) {
        bt->media.t0 = current_tick;
        bt->media.sent = 0;
    }
    restore_irq(irq);
    if (bt)
        queue_post(&bt->queue, BT_MSG_STREAM, 1);
}

static void sink_stop(void)
{
    pcm_on = false;
    pcm_left = 0;
    if (bt)
        queue_post(&bt->queue, BT_MSG_STREAM, 0);
}

static const unsigned long bt_samprs[] = { SAMPR_44 };

/* The headset's own buffer: a typical figure until AVDTP delay reports
 * tell the real one */
#define HEADSET_DELAY_MS 150

int pcm_sink_latency_ms(void)
{
    if (pcm_current_sink() != PCM_SINK_BT)
        return 0;
    return LEAD * 1000 / RATE + HEADSET_DELAY_MS;
}

struct pcm_sink bt_pcm_sink = {
    .caps = {
        .samprs       = bt_samprs,
        .num_samprs   = ARRAYLEN(bt_samprs),
        .default_freq = 0,
#ifdef WANT_SWVOL
        .volume_type  = PCM_SINK_SWVOL,
#else
        .volume_type  = PCM_SINK_HWVOL,
#endif
    },
    .ops = {
        .init     = sink_nop,
        .postinit = sink_nop,
        .set_freq = sink_set_freq,
        .lock     = sink_lock,
        .unlock   = sink_unlock,
        .play     = sink_play,
        .stop     = sink_stop,
    },
};
