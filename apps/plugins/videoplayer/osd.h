/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player on-screen display: pixel art drawn into the frame buffer
 * over the picture, the way the picture is held (the canvas).
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
#ifndef OSD_H
#define OSD_H

#include <stdbool.h>
#include <stdint.h>

/* What the player shows: filled in before each osd_draw() */
struct osd_state {
    const char *title;
    uint32_t pos;               /* ms */
    uint32_t duration;          /* ms, 0 if unknown */
    bool paused;
    bool seeking;               /* seek_ms is where a seek goes */
    uint32_t seek_ms;
    uint32_t seek_from;         /* where it started */
    int scan;                   /* fast forward (> 0) or rewind speed */
    const uint32_t *chapters;   /* their start times, ms */
    int nchapters;
    int captions;               /* 0 none to choose, 1 off, 2 on */
    bool bluetooth;             /* the sound goes there */
    const char *caption;        /* the cue on screen, or NULL */
};

void osd_init(void);
/* The bars, for a while (or while paused or seeking) */
void osd_show_bars(void);
void osd_toggle_bars(void);
void osd_show_volume(void);
/* Play/pause pop in the middle */
void osd_pop(void);
/* The side chevrons of a seek: dir -1 back, 1 forward */
void osd_seek_hint(int dir);
/* A note at the top for a while */
void osd_toast(const char *msg);

/* Draw over the picture in the frame buffer. Returns false if nothing
 * was drawn (so the screen holds only the picture). */
bool osd_draw(const struct osd_state *s);
/* Something on screen changes by itself (a slide, a timeout): the time to
 * draw again, counted from the last osd_draw() (due once the tick reaches
 * it, maybe already), or 0 */
long osd_next_change(void);
/* The same while a card is up: only the card moves */
long osd_card_next_change(void);

/* Full-screen cards over the dimmed picture */
struct osd_item {
    const char *label;
    const char *value;          /* NULL: none */
    bool adjust;                /* the value changes with left/right */
};
/* A menu or info card slides in when first drawn after osd_card_open();
 * a message is drawn once, over the screen as it is */
void osd_card_open(void);
void osd_draw_menu(const char *title, const struct osd_item *items, int n,
                   int selected);
void osd_draw_info(const char *title, const struct osd_item *items, int n);
void osd_draw_message(const char *title, const char *text);

/* Text in the OSD's font, for layout by others */
int osd_text_width(const char *s);

#endif
