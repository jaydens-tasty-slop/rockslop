/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * MPEG-1/2 video with mpegplayer's libmpeg2.
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
#ifndef MPEG_H
#define MPEG_H

#include "video.h"

/* extra: codec setup from the container (a sequence header), or NULL;
 * width x height: the largest picture */
bool mpeg_init(const uint8_t *extra, int extra_len, int width, int height);
void mpeg_reset(void);
void mpeg_feed(const uint8_t *data, uint32_t size, uint32_t ms);
bool mpeg_next(video_layout_fn *layout, bool show, uint32_t *ms);
bool mpeg_redraw(video_layout_fn *layout);
/* Frames held: 2 without B pictures' room, 3 with */
int mpeg_frames(void);

#endif
