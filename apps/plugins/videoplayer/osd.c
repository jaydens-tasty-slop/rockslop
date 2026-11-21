/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player on-screen display. Everything is placed by the pixel: a
 * 5x7 font of its own, sprites at their real size, panels of darkened
 * picture with dithered edges, slides in whole-pixel steps.
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
#include "plugin.h"
#include "video.h"
#include "osd.h"

/* ---- palette, RGB565 ---- */
#define RGB(r, g, b)    ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
#define INK             RGB( 12,  10,  24)
#define PANEL           RGB( 26,  28,  56)
#define PANEL_HI        RGB( 54,  58, 110)
#define TRACK           RGB( 74,  82, 136)
#define TRACK_LO        RGB( 42,  46,  90)
#define ACCENT          RGB(255, 178,  46)
#define ACCENT_HI       RGB(255, 226, 140)
#define ACCENT_LO       RGB(208, 106,  16)
#define TEXT            RGB(246, 236, 214)
#define TEXT_LO         RGB(196, 180, 150)
#define DIM             RGB(150, 156, 200)
#define RED             RGB(255,  90,  74)
#define GREEN           RGB( 94, 224, 138)
#define BLUE            RGB( 74, 180, 255)
/* Under a panel: a quarter of the picture over this */
#define GLASS           RGB( 16,  20,  40)

#define TOP_H           12      /* top bar rows, then two dithered ones */
#define BOT_H           26      /* bottom panel rows, two dithered above */
#define VOL_W           21
#define VOL_STEPS       12
#define SLIDE_PX        3       /* per tick */
#define BARS_TICKS      (HZ * 3)
#define VOLUME_TICKS    (HZ * 3 / 2)
#define POP_TICKS       (HZ / 2)
#define HINT_TICKS      (HZ * 3 / 4)
#define TOAST_TICKS     (HZ * 3)

/* generated: rows top to bottom, bit 0 the left pixel; rows 7 and 8 are
 * below the baseline */
static const uint8_t font_width[95] = {
    3, 1, 3, 5, 5, 5, 5, 1, 2, 2, 5, 5, 2, 4, 1, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 1, 2, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 2, 5, 2, 5, 5, 2, 5, 5, 4, 5, 5, 4, 5, 5, 1, 3, 4,
    2, 5, 5, 5, 5, 5, 4, 5, 4, 5, 5, 5, 5, 5, 5, 3, 1, 3, 5,
};
static const uint8_t font_rows[95][9] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /*   */
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00 }, /* ! */
    { 0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* " */
    { 0x00, 0x0a, 0x1f, 0x0a, 0x1f, 0x0a, 0x00, 0x00, 0x00 }, /* # */
    { 0x04, 0x1e, 0x05, 0x0e, 0x14, 0x0f, 0x04, 0x00, 0x00 }, /* $ */
    { 0x13, 0x13, 0x08, 0x04, 0x02, 0x19, 0x19, 0x00, 0x00 }, /* % */
    { 0x06, 0x09, 0x05, 0x02, 0x15, 0x09, 0x16, 0x00, 0x00 }, /* & */
    { 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* quote */
    { 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00 }, /* ( */
    { 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x00, 0x00 }, /* ) */
    { 0x00, 0x04, 0x15, 0x0e, 0x15, 0x04, 0x00, 0x00, 0x00 }, /* * */
    { 0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00, 0x00, 0x00 }, /* + */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x01 }, /* , */
    { 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* - */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 }, /* . */
    { 0x10, 0x08, 0x08, 0x04, 0x02, 0x02, 0x01, 0x00, 0x00 }, /* / */
    { 0x0e, 0x11, 0x19, 0x15, 0x13, 0x11, 0x0e, 0x00, 0x00 }, /* 0 */
    { 0x04, 0x06, 0x04, 0x04, 0x04, 0x04, 0x0e, 0x00, 0x00 }, /* 1 */
    { 0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1f, 0x00, 0x00 }, /* 2 */
    { 0x0f, 0x10, 0x10, 0x0e, 0x10, 0x10, 0x0f, 0x00, 0x00 }, /* 3 */
    { 0x09, 0x09, 0x09, 0x1f, 0x08, 0x08, 0x08, 0x00, 0x00 }, /* 4 */
    { 0x1f, 0x01, 0x0f, 0x10, 0x10, 0x10, 0x0f, 0x00, 0x00 }, /* 5 */
    { 0x0e, 0x01, 0x01, 0x0f, 0x11, 0x11, 0x0e, 0x00, 0x00 }, /* 6 */
    { 0x1f, 0x10, 0x08, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00 }, /* 7 */
    { 0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e, 0x00, 0x00 }, /* 8 */
    { 0x0e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x0e, 0x00, 0x00 }, /* 9 */
    { 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 }, /* : */
    { 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x02, 0x01, 0x00 }, /* ; */
    { 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x00, 0x00 }, /* < */
    { 0x00, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00 }, /* = */
    { 0x01, 0x02, 0x04, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 }, /* > */
    { 0x0e, 0x11, 0x10, 0x0c, 0x04, 0x00, 0x04, 0x00, 0x00 }, /* ? */
    { 0x0e, 0x11, 0x1d, 0x15, 0x1d, 0x01, 0x1e, 0x00, 0x00 }, /* @ */
    { 0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11, 0x00, 0x00 }, /* A */
    { 0x0f, 0x11, 0x11, 0x0f, 0x11, 0x11, 0x0f, 0x00, 0x00 }, /* B */
    { 0x0e, 0x11, 0x01, 0x01, 0x01, 0x11, 0x0e, 0x00, 0x00 }, /* C */
    { 0x0f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0f, 0x00, 0x00 }, /* D */
    { 0x1f, 0x01, 0x01, 0x0f, 0x01, 0x01, 0x1f, 0x00, 0x00 }, /* E */
    { 0x1f, 0x01, 0x01, 0x0f, 0x01, 0x01, 0x01, 0x00, 0x00 }, /* F */
    { 0x0e, 0x11, 0x01, 0x1d, 0x11, 0x11, 0x1e, 0x00, 0x00 }, /* G */
    { 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11, 0x00, 0x00 }, /* H */
    { 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07, 0x00, 0x00 }, /* I */
    { 0x1c, 0x08, 0x08, 0x08, 0x08, 0x09, 0x06, 0x00, 0x00 }, /* J */
    { 0x11, 0x09, 0x05, 0x03, 0x05, 0x09, 0x11, 0x00, 0x00 }, /* K */
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1f, 0x00, 0x00 }, /* L */
    { 0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11, 0x00, 0x00 }, /* M */
    { 0x11, 0x13, 0x15, 0x19, 0x11, 0x11, 0x11, 0x00, 0x00 }, /* N */
    { 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e, 0x00, 0x00 }, /* O */
    { 0x0f, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x01, 0x00, 0x00 }, /* P */
    { 0x0e, 0x11, 0x11, 0x11, 0x15, 0x09, 0x16, 0x00, 0x00 }, /* Q */
    { 0x0f, 0x11, 0x11, 0x0f, 0x05, 0x09, 0x11, 0x00, 0x00 }, /* R */
    { 0x1e, 0x01, 0x01, 0x0e, 0x10, 0x10, 0x0f, 0x00, 0x00 }, /* S */
    { 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00 }, /* T */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e, 0x00, 0x00 }, /* U */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04, 0x00, 0x00 }, /* V */
    { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1b, 0x11, 0x00, 0x00 }, /* W */
    { 0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11, 0x00, 0x00 }, /* X */
    { 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00 }, /* Y */
    { 0x1f, 0x10, 0x08, 0x04, 0x02, 0x01, 0x1f, 0x00, 0x00 }, /* Z */
    { 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00 }, /* [ */
    { 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10, 0x00, 0x00 }, /* backslash */
    { 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00 }, /* ] */
    { 0x04, 0x0a, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* ^ */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00 }, /* _ */
    { 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* ` */
    { 0x00, 0x00, 0x0e, 0x10, 0x1e, 0x11, 0x1e, 0x00, 0x00 }, /* a */
    { 0x01, 0x01, 0x0f, 0x11, 0x11, 0x11, 0x0f, 0x00, 0x00 }, /* b */
    { 0x00, 0x00, 0x0e, 0x01, 0x01, 0x01, 0x0e, 0x00, 0x00 }, /* c */
    { 0x10, 0x10, 0x1e, 0x11, 0x11, 0x11, 0x1e, 0x00, 0x00 }, /* d */
    { 0x00, 0x00, 0x0e, 0x11, 0x1f, 0x01, 0x0e, 0x00, 0x00 }, /* e */
    { 0x0c, 0x02, 0x0f, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00 }, /* f */
    { 0x00, 0x00, 0x1e, 0x11, 0x11, 0x11, 0x1e, 0x10, 0x0e }, /* g */
    { 0x01, 0x01, 0x0f, 0x11, 0x11, 0x11, 0x11, 0x00, 0x00 }, /* h */
    { 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00 }, /* i */
    { 0x04, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x02 }, /* j */
    { 0x01, 0x01, 0x09, 0x05, 0x03, 0x05, 0x09, 0x00, 0x00 }, /* k */
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00 }, /* l */
    { 0x00, 0x00, 0x0f, 0x15, 0x15, 0x15, 0x15, 0x00, 0x00 }, /* m */
    { 0x00, 0x00, 0x0f, 0x11, 0x11, 0x11, 0x11, 0x00, 0x00 }, /* n */
    { 0x00, 0x00, 0x0e, 0x11, 0x11, 0x11, 0x0e, 0x00, 0x00 }, /* o */
    { 0x00, 0x00, 0x0f, 0x11, 0x11, 0x11, 0x0f, 0x01, 0x01 }, /* p */
    { 0x00, 0x00, 0x1e, 0x11, 0x11, 0x11, 0x1e, 0x10, 0x10 }, /* q */
    { 0x00, 0x00, 0x0d, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00 }, /* r */
    { 0x00, 0x00, 0x1e, 0x01, 0x0e, 0x10, 0x0f, 0x00, 0x00 }, /* s */
    { 0x02, 0x02, 0x0f, 0x02, 0x02, 0x02, 0x0c, 0x00, 0x00 }, /* t */
    { 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x1e, 0x00, 0x00 }, /* u */
    { 0x00, 0x00, 0x11, 0x11, 0x11, 0x0a, 0x04, 0x00, 0x00 }, /* v */
    { 0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0a, 0x00, 0x00 }, /* w */
    { 0x00, 0x00, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x00, 0x00 }, /* x */
    { 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x1e, 0x10, 0x0e }, /* y */
    { 0x00, 0x00, 0x1f, 0x08, 0x04, 0x02, 0x1f, 0x00, 0x00 }, /* z */
    { 0x04, 0x02, 0x02, 0x01, 0x02, 0x02, 0x04, 0x00, 0x00 }, /* { */
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 }, /* | */
    { 0x01, 0x02, 0x02, 0x04, 0x02, 0x02, 0x01, 0x00, 0x00 }, /* } */
    { 0x00, 0x00, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* ~ */
};

/* Latin-1 letters drawn as their base letters */
static const char latin1_base[] =
    "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPs"
    "aaaaaaaceeeeiiiidnooooo/ouuuuypy";

/* Sprites: one character per pixel, '.' clear */
struct sprite {
    uint8_t w, h;
    const char *px;
};

static const struct sprite spr_cc = { 11, 7,
    ".WWWWWWWWW."
    "WWW##WW##WW"
    "WW#WWW#WWWW"
    "WW#WWW#WWWW"
    "WW#WWW#WWWW"
    "WWW##WW##WW"
    ".WWWWWWWWW." };

static const struct sprite spr_bt = { 5, 7,
    "..B.."
    "..BB."
    "B.B.B"
    ".BBB."
    "B.B.B"
    "..BB."
    "..B.." };

static const struct sprite spr_speaker = { 8, 7,
    "...W..W."
    "..WW...W"
    "WWWW.W.W"
    "WWWW.W.W"
    "WWWW.W.W"
    "..WW...W"
    "...W..W." };

static const struct sprite spr_mute = { 8, 7,
    "...W...."
    "..WW...."
    "WWWW.W.W"
    "WWWW..W."
    "WWWW.W.W"
    "..WW...."
    "...W...." };

static const struct sprite spr_warning = { 11, 9,
    ".....A....."
    "....AAA...."
    "....A#A...."
    "...AA#AA..."
    "...AA#AA..."
    "..AAA#AAA.."
    "..AAAAAAA.."
    ".AAAA#AAAA."
    "AAAAAAAAAAA" };

struct slide {
    int16_t off, target;
    long tick;
};

static const struct canvas *cv;
static int W, H;

static struct {
    long drawn;                 /* the tick osd_draw() last ran */
    long bars_until;
    struct slide bars;          /* 0 shown .. BOT_H hidden */
    long volume_until;
    struct slide volume;
    long pop_tick;
    bool popped;
    int hint_dir;
    long hint_until;
    char hint[12];              /* how far (or fast), kept once a seek lands */
    struct slide card;
    char toast[64];
    long toast_until;
} o;

/* ---- pixels ---- */

static inline fb_data pix(unsigned c)
{
#if LCD_PIXELFORMAT == RGB565SWAPPED
    return swap16(c);
#else
    return c;
#endif
}

static inline fb_data *at(int x, int y)
{
    return cv->origin + x * cv->dx + y * cv->dy;
}

static inline bool inside(int x, int y)
{
    return (unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H;
}

static void put(int x, int y, unsigned c)
{
    if (inside(x, y))
        *at(x, y) = pix(c);
}

static void fill(int x, int y, int w, int h, unsigned c)
{
    for (int j = MAX(y, 0); j < MIN(y + h, H); j++)
        for (int i = MAX(x, 0); i < MIN(x + w, W); i++)
            *at(i, j) = pix(c);
}

/* The picture as seen through the panels' dark glass */
static inline void shade_px(int x, int y)
{
    fb_data *p = at(x, y);
    unsigned v = pix(*p);       /* the swap is its own inverse */
    *p = pix(((v >> 2) & 0x39e7) + GLASS);
}

static void shade(int x, int y, int w, int h)
{
    for (int j = MAX(y, 0); j < MIN(y + h, H); j++)
        for (int i = MAX(x, 0); i < MIN(x + w, W); i++)
            shade_px(i, j);
}

/* A dithered edge row: every other pixel shaded next to a panel (d 1),
 * every fourth one further out (d 2) */
static void fringe(int y, int d)
{
    if (y < 0 || y >= H)
        return;
    for (int x = (y & 1) * d; x < W; x += 2 * d)
        shade_px(x, y);
}

/* A box with its corner pixels cut */
static void box(int x, int y, int w, int h, unsigned border, unsigned inner)
{
    fill(x + 1, y, w - 2, 1, border);
    fill(x + 1, y + h - 1, w - 2, 1, border);
    fill(x, y + 1, 1, h - 2, border);
    fill(x + w - 1, y + 1, 1, h - 2, border);
    fill(x + 1, y + 1, w - 2, h - 2, inner);
}

static unsigned sprite_colour(char c)
{
    switch (c)
    {
    case '#': return INK;
    case 'A': return ACCENT;
    case 'B': return BLUE;
    case 'D': return DIM;
    default:  return TEXT;
    }
}

static void sprite(int x, int y, const struct sprite *s)
{
    const char *p = s->px;
    for (int j = 0; j < s->h; j++)
        for (int i = 0; i < s->w; i++, p++)
            if (*p != '.')
                put(x + i, y + j, sprite_colour(*p));
}

/* Like sprite(), its body ('W') and ink ('#') in other colours */
static void sprite_tint(int x, int y, const struct sprite *s, unsigned body,
                        unsigned ink)
{
    const char *p = s->px;
    for (int j = 0; j < s->h; j++)
        for (int i = 0; i < s->w; i++, p++)
            if (*p != '.')
                put(x + i, y + j, *p == '#' ? ink : *p == 'W' ? body
                                             : sprite_colour(*p));
}

/* Play: rows 1, 3, 5 .. h, .. 3, 1 pixels wide (h odd) */
static void icon_play(int x, int y, int h, unsigned c)
{
    for (int j = 0; j < h; j++)
        fill(x, y + j, 2 * MIN(j, h - 1 - j) + 1, 1, c);
}

static void icon_pause(int x, int y, int h, unsigned c)
{
    int bar = (h + 2) / 3;
    fill(x, y, bar, h, c);
    fill(x + h - bar, y, bar, h, c);
}

/* A two-pixel chevron, 5 x 7, pointing right (dir > 0) or left */
static void chevron(int x, int y, int dir, unsigned c)
{
    for (int j = 0; j < 7; j++)
    {
        int d = MIN(j, 6 - j);
        fill(dir > 0 ? x + d : x + 3 - d, y + j, 2, 1, c);
    }
}

/* ---- text ---- */

#define ELLIPSIS_W  5

static ucschar_t fold(ucschar_t ch)
{
    if (ch >= 0xc0 && ch <= 0xff)
        return latin1_base[ch - 0xc0];
    if (ch == 0x2018 || ch == 0x2019)
        return '\'';
    if (ch == 0x201c || ch == 0x201d)
        return '"';
    if (ch == 0x2013 || ch == 0x2014)
        return '-';
    if (ch == 0xa0)
        return ' ';
    return ch;
}

/* Draws ch with its top at y (c > 0xffff: measure only). Returns its
 * advance. Glyphs not in the font come from the UI font, made 1-bit. */
static int glyph(int x, int y, ucschar_t ch, unsigned c, bool draw)
{
    ch = fold(ch);
    if (ch >= 32 && ch < 127)
    {
        int w = font_width[ch - 32];
        if (draw)
        {
            const uint8_t *rows = font_rows[ch - 32];
            for (int j = 0; j < 9; j++)
                for (int i = 0; i < w; i++)
                    if (rows[j] & (1 << i))
                        put(x + i, y + j, c);
        }
        return w + 1;
    }

    struct font *pf = rb->font_get(FONT_UI);
    int w = rb->font_get_width(pf, ch);
    if (draw)
    {
        const unsigned char *bits = rb->font_get_bits(pf, ch);
        int top = y + 7 - pf->ascent;
        for (int j = 0; j < (int)pf->height; j++)
        {
            for (int i = 0; i < w; i++)
            {
                bool on;
                if (pf->depth)
                {
                    int n = j * w + i;
                    on = ((bits[n >> 1] >> ((n & 1) * 4)) & 15) >= 8;
                }
                else
                    on = bits[(j >> 3) * w + i] & (1 << (j & 7));
                if (on)
                    put(x + i, top + j, c);
            }
        }
    }
    return w + 1;
}

/* Up to len bytes of s (len < 0: all of it) */
static int text_width_n(const char *s, int len)
{
    const unsigned char *p = (const unsigned char *)s;
    const unsigned char *end = len < 0 ? NULL : p + len;
    int w = 0;
    while (*p && (!end || p < end))
    {
        ucschar_t ch;
        p = rb->utf8decode(p, &ch);
        w += glyph(0, 0, ch, 0, false);
    }
    return w > 0 ? w - 1 : 0;
}

int osd_text_width(const char *s)
{
    return text_width_n(s, -1);
}

/* Draws len bytes (len < 0: all) of s, cut short with an ellipsis to fit
 * in maxw (0: no limit). Returns the x after it. */
static int text_n(int x, int y, const char *s, int len, unsigned c, int maxw)
{
    const unsigned char *p = (const unsigned char *)s;
    const unsigned char *end = len < 0 ? NULL : p + len;
    bool cut = maxw > 0 && text_width_n(s, len) > maxw;
    int limit = x + maxw - ELLIPSIS_W - 1;

    while (*p && (!end || p < end))
    {
        ucschar_t ch;
        p = rb->utf8decode(p, &ch);
        int adv = glyph(0, 0, ch, 0, false);
        if (cut && x + adv - 1 > limit)
            break;
        x += glyph(x, y, ch, c, true);
    }
    if (cut)
    {
        for (int i = 0; i < 3; i++)
            put(x + i * 2, y + 6, c);
        x += ELLIPSIS_W + 1;
    }
    return x;
}

static int text(int x, int y, const char *s, unsigned c)
{
    return text_n(x, y, s, -1, c, 0);
}

static void text_right(int right, int y, const char *s, unsigned c)
{
    text(right - osd_text_width(s) + 1, y, s, c);
}

/* With a one-pixel ink outline all round */
static void text_outlined(int x, int y, const char *s, int len, unsigned c)
{
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++)
            if (i || j)
                text_n(x + i, y + j, s, len, INK, 0);
    text_n(x, y, s, len, c, 0);
}

static void fmt_time(char *buf, size_t size, uint32_t ms)
{
    unsigned long s = ms / 1000;
    if (s >= 3600)
        rb->snprintf(buf, size, "%lu:%02lu:%02lu", s / 3600, s / 60 % 60, s % 60);
    else
        rb->snprintf(buf, size, "%lu:%02lu", s / 60, s % 60);
}

/* ---- animation ---- */

/* Move s towards target, SLIDE_PX pixels per tick since the last step */
static int slide_to(struct slide *s, int target)
{
    long now = *rb->current_tick;
    if (target != s->target)
    {
        s->target = target;
        s->tick = now - 1;
    }
    if (s->off != target)
    {
        int px = MAX(now - s->tick, 1) * SLIDE_PX;
        s->tick = now;
        s->off = s->off < target ? MIN(s->off + px, target)
                                 : MAX(s->off - px, target);
    }
    return s->off;
}

static inline bool sliding(const struct slide *s)
{
    return s->off != s->target;
}

void osd_init(void)
{
    rb->memset(&o, 0, sizeof(o));
    o.bars.off = o.bars.target = BOT_H;
    o.volume.off = o.volume.target = VOL_W + 2;
}

void osd_show_bars(void)
{
    o.bars_until = *rb->current_tick + BARS_TICKS;
}

void osd_toggle_bars(void)
{
    if (TIME_BEFORE(*rb->current_tick, o.bars_until))
        o.bars_until = *rb->current_tick;
    else
        osd_show_bars();
}

void osd_show_volume(void)
{
    o.volume_until = *rb->current_tick + VOLUME_TICKS;
}

void osd_pop(void)
{
    o.pop_tick = *rb->current_tick;
    o.popped = true;
}

void osd_seek_hint(int dir)
{
    o.hint_dir = dir;
    o.hint_until = *rb->current_tick + HINT_TICKS;
}

void osd_toast(const char *msg)
{
    rb->strlcpy(o.toast, msg, sizeof(o.toast));
    o.toast_until = *rb->current_tick + TOAST_TICKS;
}

static bool bars_wanted(const struct osd_state *s)
{
    return s->paused || s->seeking || s->scan ||
           TIME_BEFORE(*rb->current_tick, o.bars_until);
}

/* When a pop's wipe changes next, from its start */
static long pop_step(long t)
{
    static const uint8_t steps[] = { 2, 4, 6, POP_TICKS, POP_TICKS + 2,
                                     POP_TICKS + 4, POP_TICKS + 6 };
    for (unsigned i = 0; i < ARRAYLEN(steps); i++)
        if (t < steps[i])
            return steps[i];
    return t;
}

long osd_next_change(void)
{
    /* from the last draw, not from now: woken right at a step, now's next
       step is already the one after, and the due one never drew */
    long drawn = o.drawn, next = 0;
#define SOON(t) do { long t_ = (t); if (!next || TIME_BEFORE(t_, next)) next = t_; } while (0)
    /* a slide's next step */
    if (sliding(&o.bars))
        SOON(o.bars.tick + 1);
    if (sliding(&o.volume))
        SOON(o.volume.tick + 1);
    if (sliding(&o.card))
        SOON(o.card.tick + 1);
    /* timeouts */
    if (TIME_BEFORE(drawn, o.bars_until))
        SOON(o.bars_until);
    if (TIME_BEFORE(drawn, o.volume_until))
        SOON(o.volume_until);
    if (TIME_BEFORE(drawn, o.toast_until))
        SOON(o.toast_until);
    if (o.popped)
        SOON(o.pop_tick + pop_step(drawn - o.pop_tick));
    if (TIME_BEFORE(drawn, o.hint_until))
        SOON((drawn | 7) + 1);  /* the next chevron lit */
    else if (o.hint_dir)
        SOON(o.hint_until);     /* gone */
#undef SOON
    return next;
}

long osd_card_next_change(void)
{
    return sliding(&o.card) ? o.card.tick + 1 : 0;
}

/* ---- the bars ---- */

static void draw_battery(int x, int y)
{
    int level = rb->battery_level();
    bool charging = rb->charging_state();
    unsigned c = level >= 0 && level < 15 && !charging ? RED : TEXT;

    fill(x, y, 11, 1, c);
    fill(x, y + 6, 11, 1, c);
    fill(x, y + 1, 1, 5, c);
    fill(x + 10, y + 1, 1, 5, c);
    fill(x + 11, y + 2, 1, 3, c);
    int segs = level < 0 ? 0 : level > 66 ? 3 : level > 33 ? 2 : level >= 10 ? 1 : 0;
    if (charging)   /* the segments fill up, one a half second */
        segs = (*rb->current_tick / (HZ / 2)) % 4;
    for (int i = 0; i < segs; i++)
        fill(x + 2 + i * 3, y + 2, 2, 3, charging ? GREEN : c);
}

static void draw_top(const struct osd_state *s, int y)
{
    shade(0, y, W, TOP_H);
    fill(0, y + TOP_H - 1, W, 1, PANEL_HI);
    fringe(y + TOP_H, 1);
    fringe(y + TOP_H + 1, 2);
    fill(0, y + 2, 2, 7, ACCENT);

    int right = W - 4;
    draw_battery(right - 11, y + 2);
    right -= 16;
    if (s->bluetooth)
    {
        sprite(right - 4, y + 2, &spr_bt);
        right -= 9;
    }
    text_n(4, y + 2, s->title, -1, TEXT, right - 4);
}

/* Ticks along the bar: chapters, else every so many minutes */
static void draw_ticks(const struct osd_state *s, int x0, int w, int y)
{
    if (!s->duration)
        return;
    if (s->nchapters > 1)
    {
        for (int i = 1; i < s->nchapters; i++)
        {
            int x = x0 + (int)((uint64_t)w * s->chapters[i] / s->duration);
            fill(x, y - 1, 1, 2, TEXT);
            fill(x, y + 2, 1, 4, INK);     /* a gap in the bar */
        }
        return;
    }
    static const uint16_t minutes[] = { 1, 2, 5, 10, 15, 30, 60 };
    uint32_t step = 0;
    for (unsigned i = 0; i < ARRAYLEN(minutes); i++)
    {
        step = minutes[i] * 60000u;
        if ((uint64_t)w * step / s->duration >= 8)
            break;
    }
    for (uint32_t t = step; t < s->duration; t += step)
    {
        int x = (int)((uint64_t)w * t / s->duration);
        if (x < w - 3)
            put(x0 + x, y, DIM);
    }
}

static int bar_x(const struct osd_state *s, uint32_t ms, int x0, int w)
{
    if (!s->duration)
        return x0;
    return x0 + (int)((uint64_t)w * MIN(ms, s->duration) / s->duration);
}

/* The bar's fill, rows y .. y + 3 */
static void bar_fill(int x, int w, int y, bool done)
{
    if (w <= 0)
        return;
    if (done)
    {
        fill(x, y, w, 1, ACCENT_HI);
        fill(x, y + 1, w, 2, ACCENT);
        fill(x, y + 3, w, 1, ACCENT_LO);
    }
    else
    {
        fill(x, y, w, 1, TRACK_LO);
        fill(x, y + 1, w, 3, TRACK);
    }
}

/* Between the playing point and a seek's target: stripes */
static void bar_hatch(int x, int w, int y)
{
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < w; i++)
            put(x + i, y + j, ((x + i + j) & 3) < 2 ? ACCENT_LO : TRACK_LO);
}

static void playhead(int x, int y, bool target)
{
    unsigned body = target ? ACCENT : TEXT, side = target ? ACCENT_LO : TEXT_LO;
    box(x - 3, y, 7, 10, INK, body);
    fill(x + 2, y + 1, 1, 8, side);
    fill(x - 2, y + 8, 5, 1, side);
}

static void draw_bottom(const struct osd_state *s, int y)
{
    char t[16];
    int x0 = 5, w = W - 10;
    uint32_t head = s->seeking ? s->seek_ms : s->pos;

    fringe(y - 1, 1);
    fringe(y - 2, 2);
    shade(0, y, W, BOT_H);
    fill(0, y, W, 1, PANEL_HI);

    /* the timeline */
    box(x0 - 1, y + 5, w + 2, 6, INK, INK);
    int hx = bar_x(s, head, x0, w), px = bar_x(s, s->pos, x0, w);
    if (s->seeking && hx != px)
    {
        int a = MIN(hx, px), b = MAX(hx, px);
        bar_fill(x0, a - x0, y + 6, true);
        bar_hatch(a, b - a, y + 6);
        bar_fill(b, x0 + w - b, y + 6, false);
    }
    else
    {
        bar_fill(x0, hx - x0, y + 6, true);
        bar_fill(hx, x0 + w - hx, y + 6, false);
    }
    draw_ticks(s, x0, w, y + 3);
    if (s->duration)
        playhead(hx, y + 3, s->seeking);

    /* the times */
    int ty = y + 15;
    if (s->paused)
        icon_pause(x0, ty, 7, TEXT);
    else if (s->scan)
    {
        chevron(x0 + (s->scan < 0), ty, s->scan, ACCENT);
    }
    else
        icon_play(x0, ty, 7, TEXT);
    fmt_time(t, sizeof(t), head);
    text(x0 + 10, ty, t, s->seeking ? ACCENT : TEXT);

    int right = x0 + w - 1;
    if (s->duration)
    {
        t[0] = '-';
        fmt_time(t + 1, sizeof(t) - 1, s->duration - MIN(head, s->duration));
        text_right(right, ty, t, DIM);
        right -= osd_text_width(t) + 5;
    }
    if (s->captions)
        sprite_tint(right - 10, ty, &spr_cc, s->captions > 1 ? TEXT : TRACK,
                    s->captions > 1 ? INK : PANEL);

    /* a seek's target above the bar */
    if (s->seeking && s->duration)
    {
        fmt_time(t, sizeof(t), s->seek_ms);
        int bw = osd_text_width(t) + 6;
        int bx = MIN(MAX(hx - bw / 2, 1), W - 1 - bw);
        int by = y - 14;
        box(bx, by, bw, 11, INK, ACCENT);
        text(bx + 3, by + 2, t, INK);
        /* its pointer */
        fill(hx - 2, by + 10, 5, 1, ACCENT);
        fill(hx - 1, by + 11, 3, 1, ACCENT);
        put(hx - 3, by + 10, INK);
        put(hx + 3, by + 10, INK);
        put(hx - 2, by + 11, INK);
        put(hx + 2, by + 11, INK);
        put(hx - 1, by + 12, INK);
        put(hx, by + 12, ACCENT);
        put(hx + 1, by + 12, INK);
        put(hx, by + 13, INK);
    }
}

/* ---- the rest ---- */

static void draw_volume(int off)
{
    int vol = rb->global_status->volume;
    int min = rb->sound_min(SOUND_VOLUME), max = rb->sound_max(SOUND_VOLUME);
    int lit = max > min ? (VOL_STEPS * (vol - min) + (max - min) / 2) / (max - min) : 0;
    int h = 14 + VOL_STEPS * 4 + 12;
    int x = W - VOL_W - 3 + off, y = (H - h) / 2;
    char t[8];

    box(x, y, VOL_W, h, INK, PANEL);
    fill(x + 1, y + 1, VOL_W - 2, 1, PANEL_HI);
    sprite(x + 5, y + 4, vol <= min ? &spr_mute : &spr_speaker);
    for (int i = 0; i < VOL_STEPS; i++)
    {
        int sy = y + 14 + (VOL_STEPS - 1 - i) * 4;
        bool on = i < lit;
        fill(x + 4, sy, VOL_W - 8, 3, on ? ACCENT : TRACK_LO);
        if (on)
            fill(x + 4, sy, VOL_W - 8, 1, ACCENT_HI);
    }
    rb->snprintf(t, sizeof(t), "%d", rb->sound_val2phys(SOUND_VOLUME, vol) / 10);
    text(x + (VOL_W - osd_text_width(t)) / 2, y + h - 10, t, TEXT);
}

/* Play or pause, big, in a box that opens and closes as a wipe */
static void draw_pop(const struct osd_state *s)
{
    long t = *rb->current_tick - o.pop_tick;
    static const uint8_t open[] = { 5, 15, 25 };
    int full = 25, h;

    if (t >= POP_TICKS + 6)
    {
        o.popped = false;
        return;
    }
    if (t < 6)
        h = open[t / 2];
    else if (t < POP_TICKS)
        h = full;
    else
        h = open[2 - (t - POP_TICKS) / 2];
    int x = (W - full) / 2, y = (H - h) / 2;
    shade(x + 1, y + 1, full - 2, h - 2);
    box(x, y, full, h, INK, INK);
    shade(x + 2, y + 1, full - 4, h - 2);
    fill(x + 1, y + 1, 1, h - 2, PANEL_HI);
    if (h == full)
    {
        if (s->paused)
            icon_pause(x + 7, y + 7, 11, TEXT);
        else
            icon_play(x + 8, y + 7, 11, TEXT);
    }
}

/* Three chevrons at the side seeked to, one lit after the other, and how
 * far (or how fast) */
static void draw_hint(const struct osd_state *s)
{
    int dir = o.hint_dir;
    int cx = dir > 0 ? W * 3 / 4 : W / 4;
    int y = H / 2 - 12;
    int lit = (*rb->current_tick / 8) % 3;

    if (s->scan)
        rb->snprintf(o.hint, sizeof(o.hint), "x%d",
                     s->scan < 0 ? -s->scan : s->scan);
    else if (s->seeking)
    {
        int32_t d = s->seek_ms - s->seek_from;
        d = (d + (d < 0 ? -500 : 500)) / 1000;
        rb->snprintf(o.hint, sizeof(o.hint), "%c%ld s", d < 0 ? '-' : '+',
                     (long)(d < 0 ? -d : d));
    }
    for (int i = 0; i < 3; i++)
    {
        int n = dir > 0 ? i : 2 - i;
        int x = cx - 9 + i * 6;
        for (int j = -1; j <= 1; j++)
            for (int k = -1; k <= 1; k++)
                chevron(x + k, y + j, dir, INK);
        chevron(x, y, dir, n == lit ? ACCENT : TEXT);
    }
    text_outlined(cx - osd_text_width(o.hint) / 2, y + 11, o.hint, -1, TEXT);
}

/* Word wrap s into up to max lines of at most maxw pixels */
static int wrap(const char *s, int maxw, const char **lines, int *lens, int max)
{
    int n = 0;

    while (*s && n < max)
    {
        const char *end = s, *q = s;
        while (*q && *q != '\n')
        {
            const char *wend = q;
            while (*wend && *wend != ' ' && *wend != '\n')
                wend++;
            if (end > s && text_width_n(s, wend - s) > maxw)
                break;
            end = wend;
            q = wend;
            while (*q == ' ')
                q++;
        }
        lines[n] = s;
        lens[n++] = end - s;
        s = end;
        while (*s == ' ')
            s++;
        if (*s == '\n')
            s++;
    }
    return n;
}

/* A note at the top, with an accent stripe */
static void draw_toast(int y)
{
    int w = MIN(osd_text_width(o.toast) + 10, W - 8), x = (W - w) / 2;

    box(x, y, w, 13, INK, PANEL);
    fill(x + 1, y + 1, 2, 11, ACCENT);
    text_n(x + 6, y + 3, o.toast, -1, TEXT, w - 9);
}

/* Up to 4 lines centred, their bottom at y */
static void draw_caption(const char *cue, int y)
{
    const char *lines[4];
    int lens[4], maxw = W - 8, hard = 1;
    int n = wrap(cue, maxw, lines, lens, 4);
    char flow[160];

    /* the cue's own lines too long: flowed as one, but for dialogue
       ("- ...") lines, rather than leave a word alone on a line */
    for (const char *c = cue; *c; c++)
        hard += *c == '\n';
    if (n > hard && rb->strlen(cue) < sizeof(flow))
    {
        for (int i = 0; ; i++)
        {
            flow[i] = cue[i] == '\n' && cue[i + 1] != '-' ? ' ' : cue[i];
            if (!cue[i])
                break;
        }
        n = wrap(flow, maxw, lines, lens, 4);
    }

    for (int i = 0; i < n; i++)
    {
        int w = MIN(text_width_n(lines[i], lens[i]), maxw);
        text_outlined((W - w) / 2, y - (n - i) * 10 + 1, lines[i], lens[i], TEXT);
    }
}

bool osd_draw(const struct osd_state *s)
{
    bool drew = false;

    cv = video_canvas();
    W = cv->w;
    H = cv->h;
    o.drawn = *rb->current_tick;

    int bars = slide_to(&o.bars, bars_wanted(s) ? 0 : BOT_H);
    int top_off = bars * (TOP_H + 2) / BOT_H;
    int cap_y = H - 3;
    if (bars < BOT_H)
    {
        draw_top(s, -top_off);
        draw_bottom(s, H - BOT_H + bars);
        cap_y = H - BOT_H + bars - 3;
        drew = true;
    }
    /* the cue on screen belongs to where a seek leaves */
    if (s->caption && *s->caption && !s->seeking)
    {
        draw_caption(s->caption, cap_y);
        drew = true;
    }
    if (TIME_BEFORE(*rb->current_tick, o.toast_until))
    {
        draw_toast(TOP_H + 4 - top_off);
        drew = true;
    }
    if (o.hint_dir && !TIME_BEFORE(*rb->current_tick, o.hint_until) && !s->scan)
        o.hint_dir = 0;
    if (o.hint_dir)
    {
        draw_hint(s);
        drew = true;
    }
    if (o.popped)
    {
        draw_pop(s);
        drew = true;
    }
    int vol = slide_to(&o.volume,
                       TIME_BEFORE(*rb->current_tick, o.volume_until) ? 0 : VOL_W + 3);
    if (vol < VOL_W + 3)
    {
        draw_volume(vol);
        drew = true;
    }
    return drew;
}

/* ---- cards ---- */

#define ITEM_H      11
#define HEAD_H      15
#define PARA_GAP    4       /* between a message's paragraphs */
#define CARD_RISE   24      /* it slides up this far into place */

void osd_card_open(void)
{
    o.card.off = o.card.target = CARD_RISE;
    o.card.tick = *rb->current_tick;
}

/* The dimmed picture and an empty card for rows of content; returns its
 * top left in *x, *y */
static void card(const char *title, int w, int h, int *x, int *y)
{
    cv = video_canvas();
    W = cv->w;
    H = cv->h;

    shade(0, 0, W, H);
    *x = (W - w) / 2;
    *y = (H - h) / 2 + slide_to(&o.card, 0);
    box(*x, *y, w, h, INK, PANEL);
    fill(*x + 1, *y + 1, w - 2, 1, PANEL_HI);
    text_n(*x + 6, *y + 4, title, -1, ACCENT, w - 12);
    fill(*x + 4, *y + HEAD_H - 2, w - 8, 1, PANEL_HI);
}

void osd_draw_menu(const char *title, const struct osd_item *items, int n,
                   int selected)
{
    cv = video_canvas();
    int w = MIN(cv->w - 12, 140);
    int rows = MIN(n, (cv->h - 8 - HEAD_H - 3) / ITEM_H);
    int first = MIN(MAX(selected - rows / 2, 0), n - rows);
    int x, y;

    card(title, w, HEAD_H + rows * ITEM_H + 3, &x, &y);
    for (int i = 0; i < rows; i++)
    {
        const struct osd_item *it = &items[first + i];
        bool sel = first + i == selected;
        int iy = y + HEAD_H + i * ITEM_H;
        unsigned fg = sel ? INK : TEXT, vc = sel ? INK : DIM;
        int right = x + w - 6;

        if (sel)
        {
            box(x + 2, iy, w - 4, ITEM_H, ACCENT, ACCENT);
            fill(x + 3, iy, w - 6, 1, ACCENT_HI);
            fill(x + 3, iy + ITEM_H - 1, w - 6, 1, ACCENT_LO);
        }
        if (it->value)
        {
            int vw = osd_text_width(it->value);
            if (sel && it->adjust)
            {
                chevron(right - 4, iy + 2, 1, INK);
                right -= 8;
            }
            text_right(right, iy + 2, it->value, vc);
            right -= vw + 2;
            if (sel && it->adjust)
            {
                chevron(right - 4, iy + 2, -1, INK);
                right -= 8;
            }
            right -= 4;
        }
        text_n(x + 6, iy + 2, it->label, -1, fg, right - x - 6);
    }
    /* more above or below */
    if (first > 0)
        icon_play(x + w - 9, y + 5, 3, DIM);
    if (first + rows < n)
        icon_play(x + w - 9, y + HEAD_H + rows * ITEM_H, 3, DIM);
}

void osd_draw_info(const char *title, const struct osd_item *items, int n)
{
    cv = video_canvas();
    int w = MIN(cv->w - 8, 150);
    int rows = MIN(n, (cv->h - 8 - HEAD_H - 3) / ITEM_H);
    int lw = 0, x, y;

    for (int i = 0; i < rows; i++)
        lw = MAX(lw, osd_text_width(items[i].label));
    card(title, w, HEAD_H + rows * ITEM_H + 3, &x, &y);
    for (int i = 0; i < rows; i++)
    {
        int iy = y + HEAD_H + i * ITEM_H + 1;
        text(x + 6, iy, items[i].label, DIM);
        if (items[i].value)
            text_n(x + 6 + lw + 7, iy, items[i].value, -1, TEXT,
                   w - 12 - lw - 7);
    }
}

void osd_draw_message(const char *title, const char *msg)
{
    cv = video_canvas();
    int w = MIN(cv->w - 12, 140), maxw = w - 12;
    const char *lines[8];
    int lens[8];
    int n = wrap(msg, maxw, lines, lens, 8), paras = 0;

    for (int i = 1; i < n; i++)
        paras += lines[i][-1] == '\n';
    int h = HEAD_H + 14 + n * 10 + paras * PARA_GAP + 14, x, y;
    o.card.off = o.card.target = 0;     /* drawn once: no slide */
    card(title, w, h, &x, &y);
    sprite(x + (w - spr_warning.w) / 2, y + HEAD_H + 1, &spr_warning);
    int ty = y + HEAD_H + 14;
    for (int i = 0; i < n; i++, ty += 10)
    {
        int tx = x + 6, k = 0;
        /* a paragraph: a little apart, its "Label:" dimmed */
        if (lines[i] == msg || lines[i][-1] == '\n')
        {
            if (i > 0)
                ty += PARA_GAP;
            const char *colon = (rb->memchr)(lines[i], ':', lens[i]);
            if (colon)
            {
                k = colon + 1 - lines[i];
                tx = text_n(tx, ty, lines[i], k, DIM, maxw);
            }
        }
        text_n(tx, ty, lines[i] + k, lens[i] - k, TEXT, maxw - (tx - x - 6));
    }
    const char *hint = "Press any key";
    text(x + (w - osd_text_width(hint)) / 2, y + h - 12, hint, DIM);
}
