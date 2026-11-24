/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player sound.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "sound.h"
#include "mpa.h"
#include "stats.h"
#include "worker.h"

#define CHANNEL PCM_MIXER_CHAN_PLAYBACK

static const struct aformat *fmt;
static struct dsp_config *dsp;
static uint32_t out_rate;
static uint32_t in_rate;                /* the DSP's input rate */
static int in_channels;                 /* and channels */

/* Ring of stereo frames, as long as the memory allows; written here, read
 * by the mixer callback. The counts run on; each side keeps its place. */
static int16_t *ring;
static uint32_t ring_frames;
static volatile uint32_t ring_rd, ring_wr;
static uint32_t rd_pos, wr_pos;
static volatile uint32_t played;        /* frames handed to the mixer */
static volatile bool last_silent;       /* the mixer holds silence */
static int16_t silence[MIX_FRAME_SAMPLES * 2];

/* Decoding: from the compressed input into a queue of parts, which the
 * DSP takes into the ring. MPEG audio decodes on the second core where
 * there is one (worker.h), ahead while this core decodes the pictures;
 * then the decoder's run owns what it reads and writes while
 * worker_busy(), and the queue's places and parts pass between the cores
 * with barriers. The second core's stack and the queue come from the
 * ring's memory above RING_KEEP_MS: the ring alone plays on while a
 * picture decodes (up to ~70 ms), and the queue should hold a picture's
 * time of sound so that this core finds it decoded. With less room than
 * QUEUE_MIN parts this core waits for each part, longer than it would
 * take to decode it (on the device, beside Bluetooth), so it decodes. */
#define QUEUE_PARTS     8               /* at most */
#define QUEUE_MIN       4
#define RING_KEEP_MS    100
#define DECODER_STACK   2560            /* mpa_decode()'s ~1.9 KB, and more */

struct part {
    int16_t *pcm;
    uint32_t frames;
    uint32_t rate;                      /* 0: the format's */
    int channels;
};
static struct part parts[QUEUE_PARTS];
static uint32_t nparts;
/* Places of the parts taken here and decoded there, 0 to 2 nparts - 1
 * (full and empty differ); part q is parts[q % nparts] */
static volatile uint32_t q_rd, q_wr;
static bool holding;                    /* the DSP reads q_rd's part */

static uint8_t *in;                     /* compressed input */
static uint32_t in_size;
static volatile uint32_t in_fill;       /* written by this core... */
static volatile uint32_t in_used;       /* ...and up to here decoded */
static volatile bool ran_dry;           /* the decoder wants more input */
static uint32_t pcm_frames;             /* room in a part */
static struct dsp_buffer src;

static uint32_t start_ms;               /* nothing earlier plays */
static uint32_t base_ms;                /* the time of the ring's start */
static bool timed;                      /* base_ms is known */
static uint32_t first_ms;               /* the first packet's time */
static uint32_t drop;                   /* frames still to drop */
static bool eos;

static void get_more(const void **start, size_t *size)
{
    uint32_t avail = ring_wr - ring_rd;

    if (avail == 0)
    {
        /* underrun: the clock stops until data comes */
        if (!eos)
            stats_underrun();
        last_silent = true;
        *start = silence;
        *size = sizeof(silence);
        return;
    }
    uint32_t n = MIN(avail, ring_frames - rd_pos);
    /* At most one mixer frame, all of which the mixer has used when it
     * asks again: the writer then never overwrites what it still reads. */
    n = MIN(n, MIX_FRAME_SAMPLES);
    last_silent = false;
    *start = &ring[rd_pos * 2];
    *size = n * 4;
    rd_pos = rd_pos + n == ring_frames ? 0 : rd_pos + n;
    ring_rd += n;
    played += n;
    if (!eos)
        stats_sound_left(avail - n);
}

static const struct mixer_play_cbs mixer_cbs = {
    .get_more = get_more,
};

static uint32_t q_next(uint32_t q)
{
    return q + 1 == 2 * nparts ? 0 : q + 1;
}

static uint32_t queued(void)
{
    return (q_wr + 2 * nparts - q_rd) % (2 * nparts);
}

static uint32_t ring_free(void)
{
    uint32_t used = ring_wr - ring_rd + MIX_FRAME_SAMPLES;
    return used < ring_frames ? ring_frames - used : 0;
}

/* Input: whole blocks, at least 2 KB of them when blocks are small; MPEG
 * frames: room for one and the next's header (a start is checked against
 * that) */
static uint32_t input_size(const struct aformat *a)
{
    if (a->codec == AC_MPA)
        return MPA_MAX_FRAME + 4;
    return MAX(a->block_align, 2048 / a->block_align * a->block_align);
}

static uint32_t output_frames(const struct aformat *a)
{
    if (a->codec == AC_MPA)
        return MPA_PART_SAMPLES;
    return input_size(a) / a->block_align * a->samples_per_block;
}

size_t sound_work_size(const struct aformat *a)
{
    size_t size = ALIGN_UP(input_size(a), 4) + output_frames(a) * 2 * 2;
    if (a->codec == AC_MPA)
        size += ALIGN_UP(mpa_state_size(), 4);
    return size;
}

static void decode_run(void);

static void set_input(uint32_t rate, int channels)
{
    if (rate == in_rate && channels == in_channels)
        return;
    in_rate = rate;
    in_channels = channels;
    rb->dsp_configure(dsp, DSP_SET_FREQUENCY, rate);
    rb->dsp_configure(dsp, DSP_SET_STEREO_MODE,
                      channels == 2 ? STEREO_INTERLEAVED : STEREO_MONO);
}

bool sound_init(const struct aformat *a, void *ring_buf, size_t ring_size,
                void *work, size_t work_size)
{
    fmt = a;
    if (ring_size / 4 < 4 * MIX_FRAME_SAMPLES || work_size < sound_work_size(a))
        return false;

    in_size = input_size(a);
    pcm_frames = output_frames(a);
    in = work;
    parts[0].pcm = (int16_t *)ALIGN_UP((uintptr_t)in + in_size, 4);
    nparts = 1;
    if (a->codec == AC_MPA)
        mpa_init((uint8_t *)parts[0].pcm + pcm_frames * 4);

    out_rate = rb->mixer_get_frequency();
    size_t part = pcm_frames * 4;
    size_t keep = RING_KEEP_MS * out_rate / 1000 * 4;
    void *stack = NULL;
    if (a->codec == AC_MPA &&
        ring_size >= keep + DECODER_STACK + (QUEUE_MIN - 1) * part)
        stack = ring_buf;
    if (worker_start(decode_run, stack, DECODER_STACK))
    {
        ring_buf = (uint8_t *)ring_buf + DECODER_STACK;
        ring_size -= DECODER_STACK;
        while (nparts < QUEUE_PARTS && ring_size >= keep + part)
        {
            parts[nparts++].pcm = ring_buf;
            ring_buf = (uint8_t *)ring_buf + part;
            ring_size -= part;
        }
    }
    ring = ring_buf;
    ring_frames = ring_size / 4;

    dsp = rb->dsp_get_config(CODEC_IDX_AUDIO);
    rb->dsp_configure(dsp, DSP_SET_OUT_FREQUENCY, out_rate);
#ifdef HAVE_PITCHCONTROL
    rb->sound_set_pitch(PITCH_SPEED_100);
    rb->dsp_set_timestretch(PITCH_SPEED_100);
#endif
    rb->dsp_configure(dsp, DSP_RESET, 0);
    rb->dsp_configure(dsp, DSP_SET_SAMPLE_DEPTH, 16);
    in_rate = 0;
    set_input(a->rate ? a->rate : 44100, a->channels);
#if INPUT_SRC_CAPS != 0
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    sound_reset(0);
    return true;
}

void sound_close(void)
{
    rb->mixer_channel_stop(CHANNEL);
    worker_stop();
    rb->dsp_configure(dsp, DSP_FLUSH, 0);
}

void sound_reset(uint32_t ms)
{
    rb->mixer_channel_stop(CHANNEL);
    while (worker_busy())
        ;
    ring_rd = ring_wr = rd_pos = wr_pos = played = 0;
    last_silent = true;
    start_ms = base_ms = ms;
    timed = false;
    drop = 0;
    q_rd = q_wr = 0;
    holding = false;
    in_fill = in_used = 0;
    ran_dry = true;
    src.remcount = 0;
    eos = false;
    if (fmt->codec == AC_MPA)
        mpa_reset();
    rb->dsp_configure(dsp, DSP_FLUSH, 0);
}

/* Microsoft IMA ADPCM: per channel a header (first sample, step index),
 * then 4-byte groups of 8 samples, channels interleaved group by group.
 * QuickTime's: per channel 34-byte blocks of 64 samples (the predictor's
 * top 9 bits and the index, then the nibbles), channels one after the
 * other. */
static const int16_t ima_steps[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41,
    45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190,
    209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724,
    796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272,
    2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132,
    7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500,
    20350, 22385, 24623, 27086, 29794, 32767
};
static const int8_t ima_index[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8
};

static inline int16_t ima_sample(int *pred, int *idx, unsigned nib)
{
    int step = ima_steps[*idx];
    int diff = step >> 3;

    if (nib & 1)
        diff += step >> 2;
    if (nib & 2)
        diff += step >> 1;
    if (nib & 4)
        diff += step;
    *pred = (nib & 8) ? MAX(*pred - diff, -32768) : MIN(*pred + diff, 32767);
    *idx = MIN(MAX(*idx + ima_index[nib], 0), 88);
    return *pred;
}

static void ima_wav_block(const uint8_t *p, int16_t *out)
{
    int ch = fmt->channels;
    int pred[2], idx[2];

    for (int c = 0; c < ch; c++, p += 4)
    {
        pred[c] = (int16_t)(p[0] | (p[1] << 8));
        idx[c] = MIN(p[2], 88);
        out[c] = pred[c];
    }
    out += ch;
    for (uint32_t s = 1; s < fmt->samples_per_block; s += 8, out += 8 * ch)
    {
        for (int c = 0; c < ch; c++)
        {
            int16_t *o = out + c;
            for (int b = 0; b < 4; b++, p++, o += 2 * ch)
            {
                o[0] = ima_sample(&pred[c], &idx[c], *p & 15);
                o[ch] = ima_sample(&pred[c], &idx[c], *p >> 4);
            }
        }
    }
}

static void ima_qt_block(const uint8_t *p, int16_t *out)
{
    int ch = fmt->channels;

    for (int c = 0; c < ch; c++, p += 34)
    {
        int pred = (int16_t)((p[0] << 8) | (p[1] & 0x80));
        int idx = MIN(p[1] & 0x7f, 88);
        int16_t *o = out + c;
        for (int b = 0; b < 32; b++, o += 2 * ch)
        {
            o[0] = ima_sample(&pred, &idx, p[2 + b] & 15);
            o[ch] = ima_sample(&pred, &idx, p[2 + b] >> 4);
        }
    }
}

/* Decode the next part from the input into pt (frames 0: bytes were
 * skipped). Returns false if the input holds too little. */
static bool decode_part(struct part *pt)
{
    const uint8_t *p = in + in_used;
    uint32_t len = in_fill - in_used;
    uint32_t bytes, frames;

    pt->rate = 0;
    if (fmt->codec == AC_MPA)
    {
        struct mpa_frame f;
        bytes = mpa_decode(p, len, pt->pcm, &f);
        if (bytes == 0 && f.samples == 0)
            return false;
        frames = f.samples;
        pt->rate = f.rate;
        pt->channels = f.channels;
    }
    else
    {
        uint32_t align = fmt->block_align;
        uint32_t blocks = MIN(len / align, pcm_frames / fmt->samples_per_block);
        if (blocks == 0)
            return false;
        bytes = blocks * align;
        frames = blocks * fmt->samples_per_block;
        for (uint32_t b = 0; b < blocks; b++)
        {
            const uint8_t *bp = p + b * align;
            int16_t *o = pt->pcm + b * fmt->samples_per_block * fmt->channels;
            if (fmt->codec == AC_IMA_WAV)
                ima_wav_block(bp, o);
            else if (fmt->codec == AC_IMA_QT)
                ima_qt_block(bp, o);
        }
        if (fmt->codec == AC_PCM)
        {
            int16_t *pcm = pt->pcm;
            uint32_t n = bytes / (fmt->bits / 8);
            if (fmt->bits == 8)
                for (uint32_t i = 0; i < n; i++)
                    pcm[i] = (p[i] - 128) << 8;
            else if (fmt->big_endian)
                for (uint32_t i = 0; i < n; i++)
                    pcm[i] = (p[2 * i] << 8) | p[2 * i + 1];
            else
                rb->memcpy(pcm, p, bytes);
        }
    }
    in_used += bytes;
    pt->frames = frames;
    return true;
}

/* The decoder's run: parts into the queue until it is full or the input
 * holds too little */
static void decode_run(void)
{
    while (queued() < nparts)
    {
        struct part *pt = &parts[q_wr % nparts];
        if (!decode_part(pt))
        {
            ran_dry = true;
            return;
        }
        if (pt->frames)
        {
            worker_barrier();
            q_wr = q_next(q_wr);
        }
    }
}

/* The queue's next part to the DSP (the one it read is done). Returns
 * false if none is decoded. */
static bool next_part(void)
{
    if (holding)
    {
        worker_barrier();
        q_rd = q_next(q_rd);
        holding = false;
    }
    if (q_rd == q_wr)
        return false;
    worker_barrier();
    const struct part *pt = &parts[q_rd % nparts];
    holding = true;
    if (pt->rate)
        set_input(pt->rate, pt->channels);
    if (!timed)
    {
        /* the first samples: drop those before the start */
        if (first_ms < start_ms)
            drop = (uint64_t)(start_ms - first_ms) * in_rate / 1000;
        base_ms = MAX(first_ms, start_ms);
        timed = true;
    }
    uint32_t skip = MIN(drop, pt->frames);
    drop -= skip;
    src.remcount = pt->frames - skip;
    src.pin[0] = src.pin[1] = pt->pcm + skip * in_channels;
    src.proc_mask = 0;
    return true;
}

/* Run the pending input through the DSP into the ring. Returns false if
 * nothing moved (the ring is full). */
static bool process(void)
{
    uint32_t n = MIN(ring_free(), ring_frames - wr_pos);
    if (n == 0)
        return false;

    struct dsp_buffer dst;
    int32_t before = src.remcount;
    dst.remcount = 0;
    dst.bufcount = n;
    dst.p16out = &ring[wr_pos * 2];
    rb->dsp_process(dsp, &src, &dst, true);
    wr_pos = wr_pos + dst.remcount == ring_frames ? 0 : wr_pos + dst.remcount;
    ring_wr += dst.remcount;
    return dst.remcount > 0 || src.remcount != before;
}

/* With the decoder idle: drop the input it used up, read more if it ran
 * dry, and start its next run. Returns false once the stream has ended
 * and the decoder has had all of it. */
static bool feed(struct movie *m, struct stream *s)
{
    if (in_used)
    {
        in_fill -= in_used;
        rb->memmove(in, in + in_used, in_fill);
        in_used = 0;
    }
    while (ran_dry)
    {
        if (eos)
            return false;
        if (s->left == 0)
        {
            struct packet p;
            if (!m->ops->next(m, s, &p))
            {
                eos = true;
                return false;
            }
            if (!timed && in_fill == 0)
                first_ms = p.ms;
            continue;
        }
        if (in_fill == in_size)
        {
            in_fill = 0;    /* undecodable: start over */
            continue;
        }
        int32_t n = demux_read(s, in + in_fill, in_size - in_fill);
        if (n > 0)
        {
            in_fill += n;
            ran_dry = false;
        }
    }
    worker_run();
    return true;
}

bool sound_fill(struct movie *m, struct stream *s)
{
    bool more = true;               /* the decoder has more to give */

    for (;;)
    {
        /* the decoder works on while the queue has room (on the second
           core while this one goes on) */
        if (more && !worker_busy() && queued() < nparts)
            more = feed(m, s);
        if (src.remcount > 0)
        {
            if (!process())
                return true;        /* the ring is full */
            continue;
        }
        if (next_part())
            continue;
        if (!more)
            return false;
        if (worker_busy())
        {
            /* its next part is on the way */
            uint32_t t0 = stats_us();
            while (q_rd == q_wr && worker_busy())
                ;
            stats_time(ST_DECODER, t0);
        }
    }
}

bool sound_drained(void)
{
    return eos && src.remcount <= 0 && q_rd == q_wr && ring_wr == ring_rd;
}

void sound_play(bool play)
{
    if (rb->mixer_channel_status(CHANNEL) == CHANNEL_STOPPED)
    {
        if (!play)
            return;
        rb->mixer_channel_set_amplitude(CHANNEL, MIX_AMP_UNITY);
        rb->mixer_channel_play_data(CHANNEL, &mixer_cbs, NULL, 0);
    }
    else
        rb->mixer_channel_play_pause(CHANNEL, play);
}

uint32_t sound_clock_ms(void)
{
    rb->pcm_play_lock();
    uint32_t frames = played;
    if (!last_silent)
    {
        uint32_t waiting = rb->mixer_channel_get_bytes_waiting(CHANNEL) / 4;
        frames -= MIN(waiting, frames);
    }
    rb->pcm_play_unlock();
    return base_ms + (uint64_t)frames * 1000 / out_rate;
}

uint32_t sound_ring_ms(void)
{
    return (uint64_t)ring_frames * 1000 / out_rate;
}
