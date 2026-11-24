/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: fonts, palette and drawing primitives. Every screen draws
 * through these, so the views stay independent of the LCD depth.
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
#include "rt.h"

unsigned colour[C_COUNT];
struct metrics fnt;
static bool tiny;
static int sys_w, sys_h;

/* 3x5 glyphs, ASCII 32..95 then the specials 1..19 (see rt.h). Five
 * octal digits per glyph, top row first; 4/2/1 = left/middle/right. */
static const char glyphs[64][6] = {
    "00000", "22202", "55000", "57575", "36363", "51245", "25257", "22000",
    "12221", "42224", "05250", "02720", "00024", "00700", "00002", "11244",
    "75557", "26227", "71747", "71717", "55711", "74717", "74757", "71111",
    "75757", "75717", "02020", "02024", "12421", "07070", "42124", "71202",
    "25743", "25755", "65656", "34443", "65556", "74647", "74644", "34557",
    "55755", "72227", "11156", "55655", "44447", "57555", "65555", "25552",
    "65644", "25563", "65655", "34216", "72222", "55557", "55552", "55575",
    "55255", "55222", "71247", "64446", "44211", "32223", "25000", "00007",
};
static const char specials[20][6] = {
    "00000", "00200", "46764", "07770", "27222", "22272", "13731", "46764",
    "00000", "00000", "00000", "27772", "00000", "00000", "27072", "00000",
    "55555", "05250", "75357", "33264",
};
static const char special_ascii[20] = {
    ' ', '.', '>', '#', '^', 'v', '<', '>', ' ', ' ', ' ', 'o', ' ', ' ',
    '|', ' ', '=', 'x', '@', '*',
};

#define NGLYPH 96
static unsigned char font_bits[NGLYPH * 3];

static int glyph_index(unsigned char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c >= 32 && c < 96)
        return c - 32;
    if (c < 20)
        return 64 + c;
    return 31;  /* '?' */
}

static void build_font(void)
{
    for (int g = 0; g < NGLYPH; g++) {
        const char *def = g < 64 ? glyphs[g] : g < 84 ? specials[g - 64]
                                                      : "00000";
        for (int col = 0; col < 3; col++) {
            unsigned char bits = 0;
            for (int row = 0; row < 5; row++)
                if ((def[row] - '0') & (4 >> col))
                    bits |= 1 << row;
            font_bits[g * 3 + col] = bits;
        }
    }
}

#if LCD_DEPTH >= 16
#define RGB(r, g, b) LCD_RGBPACK(r, g, b)
static const unsigned ins_rgb[16] = {
    RGB(0xff, 0x6b, 0x6b), RGB(0xff, 0xd9, 0x3d), RGB(0x6b, 0xcb, 0x77),
    RGB(0x4d, 0x96, 0xff), RGB(0xc7, 0x7d, 0xff), RGB(0xff, 0x9f, 0x43),
    RGB(0x48, 0xdb, 0xfb), RGB(0xff, 0x6b, 0xcb), RGB(0xa3, 0xe6, 0x35),
    RGB(0xf4, 0xa2, 0x61), RGB(0x2e, 0xc4, 0xb6), RGB(0xe7, 0x6f, 0x51),
    RGB(0xb8, 0xc0, 0xff), RGB(0xff, 0xad, 0xad), RGB(0xca, 0xff, 0xbf),
    RGB(0x9b, 0xf6, 0xff),
};
#endif

static void palette_init(void)
{
#if LCD_DEPTH >= 16
    colour[C_BG] = RGB(0x10, 0x13, 0x1c);
    colour[C_HEAD] = RGB(0x08, 0x0a, 0x10);
    colour[C_BEAT] = RGB(0x17, 0x1c, 0x28);
    colour[C_BAR] = RGB(0x21, 0x28, 0x3a);
    colour[C_CURROW] = RGB(0x2a, 0x34, 0x52);
    colour[C_PLAYROW] = RGB(0x4a, 0x33, 0x0e);
    colour[C_MARK] = RGB(0x1e, 0x4d, 0x6e);
    colour[C_LINE] = RGB(0x3b, 0x42, 0x52);
    colour[C_TEXT] = RGB(0xe0, 0xe5, 0xee);
    colour[C_DIM] = RGB(0x46, 0x50, 0x64);
    colour[C_GUTTER] = RGB(0x76, 0x82, 0x9c);
    colour[C_GUTTERBAR] = RGB(0xe5, 0xc0, 0x7b);
    colour[C_ACCENT] = RGB(0xff, 0xb0, 0x00);
    colour[C_CURSOR] = RGB(0xff, 0xb0, 0x00);
    colour[C_CURTEXT] = RGB(0x10, 0x10, 0x10);
    colour[C_VOL] = RGB(0x56, 0xc8, 0xd2);
    colour[C_FX] = RGB(0xc6, 0x78, 0xdd);
    colour[C_PARAM] = RGB(0xb4, 0xa8, 0xf0);
    colour[C_OFF] = RGB(0x9a, 0xa4, 0xb8);
    colour[C_PANEL] = RGB(0x1c, 0x22, 0x33);
    colour[C_PANELHI] = RGB(0x33, 0x3e, 0x5c);
    colour[C_WARN] = RGB(0xff, 0x55, 0x55);
#elif LCD_DEPTH > 1
    /* greyscale: light background like the rest of Rockbox */
    for (int i = 0; i < C_COUNT; i++)
        colour[i] = LCD_BLACK;
    colour[C_BG] = colour[C_PANEL] = LCD_WHITE;
    colour[C_HEAD] = colour[C_BEAT] = colour[C_PANELHI] = LCD_LIGHTGRAY;
    colour[C_BAR] = colour[C_MARK] = LCD_LIGHTGRAY;
    colour[C_CURROW] = colour[C_PLAYROW] = LCD_LIGHTGRAY;
    colour[C_DIM] = colour[C_LINE] = colour[C_GUTTER] = LCD_DARKGRAY;
    colour[C_CURTEXT] = LCD_WHITE;
#else
    /* mono: 0 white, 1 black */
    for (int i = 0; i < C_COUNT; i++)
        colour[i] = 1;
    colour[C_BG] = colour[C_PANEL] = colour[C_HEAD] = colour[C_BEAT] = 0;
    colour[C_BAR] = colour[C_CURROW] = colour[C_PLAYROW] = 0;
    colour[C_CURTEXT] = colour[C_PANELHI] = 0;
#endif
}

unsigned ins_colour(int ins)
{
#if LCD_DEPTH >= 16
    return ins > 0 ? ins_rgb[(ins - 1) & 15] : colour[C_TEXT];
#else
    (void)ins;
    return colour[C_TEXT];
#endif
}

void draw_init(int size)
{
    build_font();
    palette_init();
    struct font *sf = rb->font_get(FONT_SYSFIXED);
    sys_w = sf->maxwidth;
    sys_h = sf->height;
    if (size == 0)  /* auto: the system font once the screen is roomy */
        size = (LCD_WIDTH >= 240 && LCD_HEIGHT >= 200) ? 2 : 1;
    tiny = size != 2;
    if (tiny) {
        fnt.w = 4;
        fnt.h = 5;
        fnt.line = 7;
    } else {
        fnt.w = sys_w;
        fnt.h = sys_h;
        fnt.line = sys_h + 2;
    }
    rb->lcd_setfont(FONT_SYSFIXED);
}

#if LCD_DEPTH > 1
static void set_fg(unsigned c)
{
    rb->lcd_set_foreground(c);
    rb->lcd_set_drawmode(DRMODE_FG);
}
#else
static void set_fg(unsigned c)
{
    /* colour 0 clears the glyph's pixels (INVERSEVID swaps set/unset) */
    rb->lcd_set_drawmode(c ? DRMODE_FG : DRMODE_BG | DRMODE_INVERSEVID);
}
#endif

void fill(int x, int y, int w, int h, unsigned c)
{
    if (w <= 0 || h <= 0)
        return;
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(c);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#else
    rb->lcd_set_drawmode(c ? DRMODE_SOLID : DRMODE_SOLID | DRMODE_INVERSEVID);
#endif
    rb->lcd_fillrect(x, y, w, h);
}

void hline(int x0, int x1, int y, unsigned c)
{
    fill(MIN(x0, x1), y, ABS(x1 - x0) + 1, 1, c);
}

void vline(int x, int y0, int y1, unsigned c)
{
    fill(x, MIN(y0, y1), 1, ABS(y1 - y0) + 1, c);
}

void pixel(int x, int y, unsigned c)
{
    fill(x, y, 1, 1, c);
}

void frame(int x, int y, int w, int h, unsigned c)
{
    hline(x, x + w - 1, y, c);
    hline(x, x + w - 1, y + h - 1, c);
    vline(x, y, y + h - 1, c);
    vline(x + w - 1, y, y + h - 1, c);
}

int textn(int x, int y, const char *s, int n, unsigned c)
{
    set_fg(c);
    int x0 = x;
    if (tiny) {
        for (int i = 0; i < n && s[i]; i++) {
            unsigned char ch = s[i];
            if (ch != ' ')
                rb->lcd_mono_bitmap_part(font_bits, glyph_index(ch) * 3, 0,
                                         NGLYPH * 3, x, y, 3, 5);
            x += 4;
        }
    } else {
        char buf[64];
        int k = 0;
        for (int i = 0; i < n && s[i] && k < (int)sizeof(buf) - 1; i++) {
            unsigned char ch = s[i];
            buf[k++] = ch < 20 ? special_ascii[ch] : ch;
        }
        buf[k] = 0;
        rb->lcd_putsxy(x, y, buf);
        x += k * sys_w;
    }
    return x - x0;
}

int text(int x, int y, const char *s, unsigned c)
{
    return textn(x, y, s, 1 << 30, c);
}

int text_width(const char *s)
{
    return rb->strlen(s) * fnt.w;
}

void draw_update(void)
{
    rb->lcd_update();
}
