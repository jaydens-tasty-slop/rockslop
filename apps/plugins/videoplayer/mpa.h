/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * MPEG-1/2 audio layer I and II decoder (MP2, the sound of .mpg files).
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
#ifndef MPA_H
#define MPA_H

#include <stddef.h>
#include <stdint.h>

#define MPA_MAX_FRAME    1729   /* layer II, 384 kbit/s at 32 kHz, padded */
#define MPA_PART_SAMPLES 384    /* per channel and call */

struct mpa_frame {
    int channels;
    uint32_t rate;
    int samples;                /* per channel */
};

size_t mpa_state_size(void);
void mpa_init(void *state);
/* Forget the history: the next frame starts a new stream */
void mpa_reset(void);
/* Decode the next part of the first frame in buf into pcm (interleaved,
 * f->samples per channel); buf keeps the frame until its last part, and has
 * room for 3 bytes after it (read, unused). Returns the bytes used up: the
 * frame's after its last part, else 0. Without samples, the bytes returned
 * were skipped (no frame start there, or a layer this does not decode), and
 * 0 means buf holds no whole frame yet. */
int mpa_decode(const uint8_t *buf, int len, int16_t *pcm, struct mpa_frame *f);

#endif
