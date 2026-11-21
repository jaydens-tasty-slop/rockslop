/* Tamagotchi P1 screen drawing (GPL-2.0-or-later, see LICENSE). */
#ifndef GOTCHI_VIEW_H
#define GOTCHI_VIEW_H

#include <stdint.h>

#define VIEW_COLS 32
#define VIEW_ROWS 16
#define VIEW_ICONS 8

/* Where the pet LCD sits on the 128x160 display. */
#define VIEW_PANEL_Y 14
#define VIEW_PANEL_H 100

struct tama_screen {
    uint32_t row[VIEW_ROWS]; /* bit x of row[y]: dot lit */
    uint8_t icon;            /* bit n: icon n selected */
};

/* Provided by the caller: fill a rectangle with 0xRRGGBB. */
void gotchi_fill(int x, int y, int w, int h, uint32_t rgb);

void gotchi_draw(const struct tama_screen *s);

#endif
