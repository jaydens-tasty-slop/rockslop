/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_FONTS_H
#define FLIPPER_FONTS_H
#include <stdint.h>

/* Glyph rows are left aligned in a uint16_t (MSB = leftmost pixel). */
struct flip_glyph {
    uint8_t dwidth, w, h;
    int8_t xoff, yoff;
    uint16_t row;           /* index of the first row in the font's rows[] */
};

struct flip_font {
    uint8_t ascent, height;
    const struct flip_glyph *glyphs;    /* ASCII 32..126 */
    const uint16_t *rows;
};

extern const struct flip_font flip_font_secondary;
extern const struct flip_font flip_font_primary;
extern const struct flip_font flip_font_keyboard;
#endif
