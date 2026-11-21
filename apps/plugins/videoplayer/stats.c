/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player: playback statistics for development builds, two lines
 * every 10 s while playing and at the end of a file (end= instead of t=):
 *   pics t=S shown=N drop=N late=N/MS dec=AVG/MAX lcd=AVG idle=PCT [bad=N]
 *   sound t=S under=N/MS low=MS/MS cpu=PCT [c1=PCT/PCT] read=MAX/N lat=MS+MS
 * late: pictures shown over half a frame after their time / the worst;
 * dec: ms to decode a picture for the screen (with those dropped on the
 * way and the reads); lcd: ms per screen update; idle and cpu: shares of
 * the playback loop's time spent waiting and on the sound. under: sound
 * underruns / ms of silence they played; low: the least sound left in the
 * ring / what it holds; c1: the second core's share of the time in the
 * sound decoder / the loop's share waiting for it; read: the slowest file
 * read (ms) / reads over 20 ms; lat: the output's latency (Bluetooth's) +
 * the menu's delay. No sound line without sound.
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
#include "stats.h"
#include "worker.h"
#ifdef YP3_DEV
#include "mmio-sl6801.h"
#endif
#ifdef HAVE_DEVLOG
#define REPORT(...) rb->dev_msg("video", __VA_ARGS__)
#else
#define REPORT(fmt, ...) DEBUGF("video " fmt "\n", __VA_ARGS__)
#endif

#define REPORT_EVERY    (10 * HZ)
#define STALL_US        20000

static struct {
    long start, last_report;
    uint64_t sum[ST_COUNT];
    uint32_t n[ST_COUNT], max[ST_COUNT];
    uint32_t stalls;
    uint32_t c1_base;                   /* worker_busy_us() at the start */
    uint32_t late;
    int32_t late_max;
    /* the mixer's, from its callback */
    volatile uint32_t underruns, silent_frames, low_frames;
    volatile bool silent;
} s;

void stats_begin(void)
{
    rb->memset(&s, 0, sizeof(s));
    s.low_frames = UINT32_MAX;
    s.start = s.last_report = *rb->current_tick;
    s.c1_base = worker_busy_us();
}

uint32_t stats_us(void)
{
#ifdef YP3_DEV
    return sl6801_us();
#else
    return *rb->current_tick * (1000000 / HZ);
#endif
}

void stats_time(enum stats_time what, uint32_t t0)
{
    uint32_t us = stats_us() - t0;

    s.sum[what] += us;
    s.n[what]++;
    s.max[what] = MAX(s.max[what], us);
    if (what == ST_READ && us >= STALL_US)
        s.stalls++;
}

void stats_late(int32_t late_ms, uint32_t frame_ms)
{
    if (late_ms > (int32_t)frame_ms / 2)
        s.late++;
    s.late_max = MAX(s.late_max, late_ms);
}

void stats_sound_left(uint32_t frames)
{
    s.silent = false;
    if (frames < s.low_frames)
        s.low_frames = frames;
}

void stats_seek(uint32_t ms, uint32_t pic_ms, int skipped, uint32_t t0)
{
    REPORT("seek to=%lu pic=%lu skipped=%d ms=%lu", (unsigned long)ms,
           (unsigned long)pic_ms, skipped, (unsigned long)(stats_us() - t0) / 1000);
}

void stats_underrun(void)
{
    if (!s.silent)
        s.underruns++;
    s.silent = true;
    s.silent_frames += MIX_FRAME_SAMPLES;
}

/* Tenths of a ms per event */
static unsigned avg10(enum stats_time t)
{
    return s.n[t] ? s.sum[t] / s.n[t] / 100 : 0;
}

void stats_report(bool end, uint32_t shown, uint32_t dropped, uint32_t bad,
                  uint32_t ring_ms, int latency_ms, int delay_ms)
{
    long now = *rb->current_tick;
    char t[16], extra[16] = "";

    if (!end && TIME_BEFORE(now, s.last_report + REPORT_EVERY))
        return;
    s.last_report = now;
    rb->snprintf(t, sizeof(t), "%s=%ld", end ? "end" : "t",
                 (now - s.start) / HZ);

    uint64_t loop = s.sum[ST_VIDEO] + s.sum[ST_SOUND] + s.sum[ST_LCD] +
                    s.sum[ST_WAIT];
    unsigned idle = loop ? s.sum[ST_WAIT] * 100 / loop : 0;
    unsigned cpu = loop ? s.sum[ST_SOUND] * 100 / loop : 0;
    unsigned dec = avg10(ST_VIDEO), lcd = avg10(ST_LCD);
    if (bad)
        rb->snprintf(extra, sizeof(extra), " bad=%u", (unsigned)bad);
    REPORT("pics %s shown=%u drop=%u late=%u/%d dec=%u.%u/%u lcd=%u.%u "
           "idle=%u%s", t, (unsigned)shown, (unsigned)dropped,
           (unsigned)s.late, (int)s.late_max, dec / 10, dec % 10,
           (unsigned)(s.max[ST_VIDEO] + 500) / 1000, lcd / 10, lcd % 10,
           idle, extra);
    if (!ring_ms)
        return;

    /* the second core: its share of the time in the decoder, and this
       core's waiting for it */
    char c1[24] = "";
    uint32_t busy = worker_busy_us();
    if (busy < s.c1_base)
        s.c1_base = 0;                  /* restarted, for another track */
    uint64_t elapsed = (uint64_t)(now - s.start) * (1000000 / HZ);
    if ((busy != s.c1_base || s.n[ST_DECODER]) && elapsed && loop)
        rb->snprintf(c1, sizeof(c1), " c1=%u/%u",
                     (unsigned)((busy - s.c1_base) * 100ull / elapsed),
                     (unsigned)(s.sum[ST_DECODER] * 100 / loop));

    unsigned rate = rb->mixer_get_frequency();
    char low[12] = "-";
    if (s.low_frames != UINT32_MAX)
        rb->snprintf(low, sizeof(low), "%u",
                     (unsigned)((uint64_t)s.low_frames * 1000 / rate));
    REPORT("sound %s under=%u/%u low=%s/%u cpu=%u%s read=%u/%u lat=%d%+d", t,
           (unsigned)s.underruns,
           (unsigned)((uint64_t)s.silent_frames * 1000 / rate), low,
           (unsigned)ring_ms, cpu, c1,
           (unsigned)(s.max[ST_READ] + 500) / 1000, (unsigned)s.stalls,
           latency_ms, delay_ms);
}
