/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player: playback statistics for development builds (sound
 * underruns, late pictures, where the time goes). HAVE_DEVLOG builds report
 * them to the dev message list (yp3dev/torture.py reads it), the
 * simulator to its console; other builds compile them out.
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
#ifndef STATS_H
#define STATS_H

#include <stdbool.h>
#include <stdint.h>

#if defined(HAVE_DEVLOG) || defined(SIMULATOR)
#define VIDEO_STATS
#endif

/* The playback loop's time, the file reads within it, and the sound's
 * waits for the decoder on the second core */
enum stats_time { ST_VIDEO, ST_SOUND, ST_LCD, ST_WAIT, ST_READ, ST_DECODER,
                  ST_COUNT };

#ifdef VIDEO_STATS
/* A new file plays */
void stats_begin(void);
/* Microseconds, to pass to stats_time() */
uint32_t stats_us(void);
/* The time since t0 (stats_us()) went to `what` */
void stats_time(enum stats_time what, uint32_t t0);
/* A picture went to the screen late_ms after its time */
void stats_late(int32_t late_ms, uint32_t frame_ms);
/* The mixer took sound and left `frames` in the ring; or found it empty
 * before the end */
void stats_sound_left(uint32_t frames);
void stats_underrun(void);
/* Every 10 s while playing, and at the end; ring_ms 0: no sound */
void stats_report(bool end, uint32_t shown, uint32_t dropped, uint32_t bad,
                  uint32_t ring_ms, int latency_ms, int delay_ms);
/* A seek to ms put the picture at pic_ms on screen (0: none) after decoding
 * `skipped` pictures it did not show; t0 from stats_us() before it. Keys
 * wait meanwhile: a long one eats a held key's repeats */
void stats_seek(uint32_t ms, uint32_t pic_ms, int skipped, uint32_t t0);
#else
static inline void stats_begin(void) {}
static inline uint32_t stats_us(void) { return 0; }
static inline void stats_time(enum stats_time what, uint32_t t0)
    { (void)what; (void)t0; }
static inline void stats_late(int32_t late_ms, uint32_t frame_ms)
    { (void)late_ms; (void)frame_ms; }
static inline void stats_sound_left(uint32_t frames) { (void)frames; }
static inline void stats_underrun(void) {}
#define stats_report(...) do { } while (0)    /* (arguments unevaluated) */
static inline void stats_seek(uint32_t ms, uint32_t pic_ms, int skipped, uint32_t t0)
    { (void)ms; (void)pic_ms; (void)skipped; (void)t0; }
#endif

#endif /* STATS_H */
