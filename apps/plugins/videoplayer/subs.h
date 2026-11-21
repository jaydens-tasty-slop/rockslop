/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player captions: the file's text tracks, and .srt / .vtt files
 * named like it next to it.
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
#ifndef SUBS_H
#define SUBS_H

#include "demux.h"

/* Find the captions for the movie at path (fd: one more handle on it);
 * its text tracks are read up to a little past where video is */
void subs_init(struct movie *m, const struct stream *video, const char *path,
               int fd);
void subs_close(void);
/* Choices: 0 .. subs_count() - 1 */
int subs_count(void);
void subs_label(int i, char *buf, size_t size);
/* The one to show when captions are on, or -1 if none */
int subs_default(void);
/* Show choice i (-1: none) from ms on */
void subs_select(int i, uint32_t ms);
int subs_selected(void);
/* The stream to seek along with the others, if a track is shown */
struct stream *subs_stream(void);
/* After a seek (the stream is put in place by then) */
void subs_seek(uint32_t ms);
/* The cue to show at ms, or NULL */
const char *subs_at(uint32_t ms);

#endif
