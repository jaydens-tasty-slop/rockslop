/* RUMBLO rendering: the 320x240 arena shrunk to 128x96 (LGPL-3.0). */
#ifndef RUMBLO_VIEW_H
#define RUMBLO_VIEW_H

#include "game.h"

#define VIEW_W 128
#define VIEW_H 96

typedef unsigned short pixel_t; /* the LCD pixel format, via view_init() */

/* pack(r, g, b) converts to the LCD pixel format. */
void view_init(pixel_t (*pack)(int r, int g, int b));
/* Draw a map screen (assets.h SCREEN_*) into the 128x96 buffer. */
void view_screen(pixel_t *dst, int screen);
/* Add the fighters, shields and weapons. */
void view_fighters(pixel_t *dst, const struct game *g);

#endif
