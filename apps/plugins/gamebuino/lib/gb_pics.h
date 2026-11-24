/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Full-screen RGB565 pictures kept in one card file (the games' pixel
 * arrays are too big for the plugin image): picture `pic` is w*h pixels,
 * pictures back to back (tools/mkpics16.py). Reads come straight from the
 * file; a missing file reads as black. */
#ifndef GB_PICS_H
#define GB_PICS_H
#include "gb_glue.h"
#include <string.h>

struct GbPics {
    const char *file;
    uint16_t w, h;
    int fd;     /* -2: not opened yet */

    /* n pixels of picture `pic` starting at (x, y) */
    void read(uint16_t *dst, uint8_t pic, uint16_t x, uint16_t y, uint16_t n)
    {
        if (fd == -2)
            fd = gb_file_open(file);
        if (fd < 0 ||
            gb_file_seek(fd, (((long)pic * h + y) * w + x) * 2, 0) < 0 ||
            gb_file_read(fd, dst, n * 2) != (int)(n * 2))
            memset(dst, 0, n * 2);
    }
};
#endif
