/* Tamagotchi P1 screen drawing: the 32x16 dot matrix is drawn at 4x4 with a
 * one pixel gap, the eight icons on a row above it with a selector arrow
 * over the active one. GPL-2.0-or-later, see LICENSE. */
#include "gotchi_view.h"
#include "icons.h"

#define COL_BEZEL 0x202820
#define COL_BG    0xb4c496
#define COL_INK   0x1c2418
#define COL_ICON  0x6c7c58

static void draw_icon(int n, int x, int y)
{
    for (int r = 0; r < 9; r++) {
        const uint8_t *p = icon_bitmaps + n * 18 + r * 2;
        unsigned bits = p[0] | (p[1] << 8);
        for (int c = 0; c < 9; c++) {
            if (bits & (1u << c))
                gotchi_fill(x + c, y + r, 1, 1, COL_ICON);
        }
    }
}

static void draw_arrow(int x, int y)
{
    gotchi_fill(x + 1, y, 5, 1, COL_INK);
    gotchi_fill(x + 2, y + 1, 3, 1, COL_INK);
    gotchi_fill(x + 3, y + 2, 1, 1, COL_INK);
}

void gotchi_draw(const struct tama_screen *s)
{
    gotchi_fill(0, 0, 128, VIEW_PANEL_Y, COL_BEZEL);
    gotchi_fill(0, VIEW_PANEL_Y, 128, VIEW_PANEL_H, COL_BG);
    gotchi_fill(0, VIEW_PANEL_Y + VIEW_PANEL_H, 128, 2, COL_BEZEL);

    for (int i = 0; i < VIEW_ICONS; i++) {
        draw_icon(i, i * 16 + 4, VIEW_PANEL_Y + 10);
        if (s->icon & (1u << i))
            draw_arrow(i * 16 + 5, VIEW_PANEL_Y + 4);
    }

    /* 4x4 pixels per dot, 3x3 of them inked */
    for (int y = 0; y < VIEW_ROWS; y++) {
        for (int x = 0; x < VIEW_COLS; x++) {
            if (s->row[y] & (1u << x))
                gotchi_fill(x * 4, VIEW_PANEL_Y + 24 + y * 4, 3, 3, COL_INK);
        }
    }
}
