/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Control hints for ported games whose view does not fill the screen:
 * black background, small muted off-white text bottom-aligned under the
 * view, the same in every compat layer.
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
#ifndef HINT_STRIP_H
#define HINT_STRIP_H

#include "plugin.h"

#define HINT_STRIP_FG    LCD_RGBPACK(150, 146, 136)
#define HINT_STRIP_LINE  9      /* FONT_SYSFIXED height + 1 */

/* Clear the whole screen to black, then draw hint lines ('|' or '\n'
 * separated) bottom-aligned, but never above y_min (the bottom of the
 * game view). Leaves the colours at off-white on black and FONT_SYSFIXED
 * selected; the caller updates the LCD. */
static inline void hint_strip_draw(const char *hint, int y_min)
{
    rb->lcd_set_viewport(NULL);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_foreground(HINT_STRIP_FG);
    if (hint && *hint) {
        int lines = 1;
        for (const char *p = hint; *p; p++)
            if (*p == '|' || *p == '\n')
                lines++;
        int y = LCD_HEIGHT - lines * HINT_STRIP_LINE;
        if (y < y_min + 1)
            y = y_min + 1;
        while (*hint && y + HINT_STRIP_LINE - 1 <= LCD_HEIGHT) {
            char line[48];
            size_t n = 0;
            while (hint[n] && hint[n] != '|' && hint[n] != '\n' &&
                   n < sizeof(line) - 1)
                n++;
            rb->memcpy(line, hint, n);
            line[n] = '\0';
            rb->lcd_putsxy(2, y, (const unsigned char *)line);
            y += HINT_STRIP_LINE;
            hint += n;
            if (*hint == '|' || *hint == '\n')
                hint++;
        }
    }
}

#endif /* HINT_STRIP_H */
