/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_CANVAS_H
#define FLIPPER_CANVAS_H
#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { ColorWhite = 0x00, ColorBlack = 0x01, ColorXOR = 0x02 } Color;
typedef enum { AlignLeft, AlignRight, AlignTop, AlignBottom, AlignCenter } Align;
typedef enum {
    FontPrimary,
    FontSecondary,
    FontKeyboard,
    FontBigNumbers,
    FontTotalNumber,
    FontMicro = 100,    /* internal: stands in for custom u8g2 fonts */
} Font;
typedef enum {
    CanvasOrientationHorizontal,
    CanvasOrientationHorizontalFlip,
    CanvasOrientationVertical,
    CanvasOrientationVerticalFlip,
} CanvasOrientation;
typedef enum { CanvasDirectionLeftToRight, CanvasDirectionTopToBottom } CanvasDirection;

typedef struct {
    uint8_t width, height;
    uint8_t frame_count, frame_rate;
    const uint8_t* const* frames;
} Icon;

typedef struct Canvas Canvas;

void canvas_reset(Canvas* canvas);
void canvas_clear(Canvas* canvas);
void canvas_commit(Canvas* canvas);
uint8_t* canvas_get_buffer(Canvas* canvas);
size_t canvas_get_buffer_size(const Canvas* canvas);
size_t canvas_width(const Canvas* canvas);
size_t canvas_height(const Canvas* canvas);
size_t canvas_current_font_height(const Canvas* canvas);
void canvas_set_color(Canvas* canvas, Color color);
void canvas_set_bitmap_mode(Canvas* canvas, bool alpha);
void canvas_set_font(Canvas* canvas, Font font);
/* Custom u8g2 fonts are not interpreted: a 3x5 digit font is used instead. */
void canvas_set_custom_u8g2_font(Canvas* canvas, const uint8_t* font);
void canvas_set_font_direction(Canvas* canvas, CanvasDirection dir);
void canvas_invert_color(Canvas* canvas);
uint16_t canvas_string_width(Canvas* canvas, const char* str);
uint8_t canvas_glyph_width(Canvas* canvas, uint16_t symbol);
void canvas_draw_str(Canvas* canvas, int32_t x, int32_t y, const char* str);
void canvas_draw_str_aligned(
    Canvas* canvas, int32_t x, int32_t y, Align horizontal, Align vertical, const char* str);
void canvas_draw_dot(Canvas* canvas, int32_t x, int32_t y);
void canvas_draw_line(Canvas* canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void canvas_draw_box(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h);
void canvas_draw_frame(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h);
void canvas_draw_rframe(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h, size_t r);
void canvas_draw_rbox(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h, size_t r);
void canvas_draw_circle(Canvas* canvas, int32_t x, int32_t y, size_t r);
void canvas_draw_disc(Canvas* canvas, int32_t x, int32_t y, size_t r);
void canvas_draw_triangle(
    Canvas* canvas, int32_t x, int32_t y, size_t base, size_t height, CanvasDirection dir);
void canvas_draw_xbm(
    Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h, const uint8_t* bitmap);
void canvas_draw_icon(Canvas* canvas, int32_t x, int32_t y, const Icon* icon);
typedef enum { IconRotation0, IconRotation90, IconRotation180, IconRotation270 } IconRotation;
/* Rotation is clockwise; x, y is the top left of the rotated icon. */
void canvas_draw_icon_ex(Canvas* canvas, int32_t x, int32_t y, const Icon* icon, IconRotation rotation);

#ifdef __cplusplus
}
#endif
#endif
