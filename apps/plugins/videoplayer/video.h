/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player pictures: decoding into the frame buffer, turned so that
 * landscape pictures fill the screen.
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
#ifndef VIDEO_H
#define VIDEO_H

#include "demux.h"

enum rotation {
    ROTATE_CW = 0,  /* landscape: the picture's top at the screen's right */
    ROTATE_CCW,     /* landscape: the picture's top at the screen's left */
    ROTATE_NEVER,
};

/* Where a picture goes: scaled by 1 / 2^shift, cropped to disp_w x disp_h
 * at (crop_x, crop_y). Its top left pixel is at row0, the next one along
 * x at row0 + dx, along y at row0 + dy. */
struct video_target {
    fb_data *row0;
    int dx, dy;
    int shift;
    int crop_x, crop_y;
    int disp_w, disp_h;
};

/* Called with a picture's size from its header: where it goes, or NULL */
typedef const struct video_target *video_layout_fn(int width, int height);

/* The screen as the picture is held: pixel (x, y) is at
 * origin + x * dx + y * dy */
struct canvas {
    fb_data *origin;
    int dx, dy;
    int w, h;
};

/* Sets up the decoder, its memory from player_alloc() */
bool video_init(const struct track *t, const uint8_t *extra, int extra_len,
                enum rotation rotation);
void video_close(void);
void video_set_rotation(enum rotation rotation);
/* After a seek: forget the pictures held for prediction */
void video_reset(void);
/* Clear the screen and draw the next picture's borders anew */
void video_clear(void);

/* Whole-packet codecs (MJPEG, raw): decode one into the frame buffer.
 * Returns false on bad data. */
bool video_decode(const uint8_t *data, uint32_t size);
/* Codecs with prediction (MPEG): video_feed() a packet's data (in pieces
 * if need be, the first with its time, the others NO_TIME), which must
 * stay in place until video_next() returns false; each true return has
 * put a picture in the frame buffer (unless !show), its time in *ms.
 * video_feed(NULL, 0, 0) at the end gives the last picture. */
#define NO_TIME UINT32_MAX
bool video_streamed(void);
void video_feed(const uint8_t *data, uint32_t size, uint32_t ms);
bool video_next(bool show, uint32_t *ms);

/* The current picture again (to draw something else over it anew) */
bool video_redraw(void);
/* Whether the picture is turned, i.e. the screen is used landscape */
bool video_turned(void);
/* (before video_init(): the screen as it stands) */
const struct canvas *video_canvas(void);
/* The picture's size and how it shows, for the file info */
void video_describe(char *buf, size_t size);

#endif /* VIDEO_H */
