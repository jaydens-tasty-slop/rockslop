/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player sound: decoding into a ring the mixer plays from. The
 * sound sets the clock.
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
#ifndef SOUND_H
#define SOUND_H

#include "demux.h"

size_t sound_work_size(const struct aformat *a);
/* ring: decoded 16-bit stereo at the mixer rate; work: compressed input
 * and decoder state, sound_work_size() bytes */
bool sound_init(const struct aformat *a, void *ring, size_t ring_size,
                void *work, size_t work_size);
void sound_close(void);

/* Empty the ring; the next packets play from start_ms on (earlier
 * samples are dropped) */
void sound_reset(uint32_t start_ms);
/* Decode until the ring is full. Returns false once the stream has ended. */
bool sound_fill(struct movie *m, struct stream *s);
/* The stream has ended and the ring has played out */
bool sound_drained(void);

void sound_play(bool play);
/* The time of the sample playing now */
uint32_t sound_clock_ms(void);
/* How much the ring holds */
uint32_t sound_ring_ms(void);

#endif /* SOUND_H */
