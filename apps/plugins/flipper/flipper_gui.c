/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Flipper Zero compatibility layer: Canvas, ViewPort and Gui.
 *
 * The canvas is the Flipper's 1 bit framebuffer (pages of 8 vertical pixels,
 * 1 KiB) and is expanded to RGB565 row by row straight into the LCD
 * framebuffer, centred on the screen. */
#include "plugin.h"
#include <furi.h>
#include <gui/gui.h>
#include "flipper_fonts.h"
#include "flipper_game.h"
#include "flipper_internal.h"
#include "lib/hint_strip.h"

#define CANVAS_BYTES (128 * 64 / 8)

#define COL_FG  LCD_RGBPACK(0x20, 0x10, 0x00)
#define COL_BG  LCD_RGBPACK(0xFF, 0x8B, 0x00)
#define COL_MID LCD_RGBPACK(0x90, 0x4D, 0x00)

struct Canvas {
    uint8_t buf[CANVAS_BYTES];
    uint8_t prev[CANVAS_BYTES];     /* previous presented frame (blend mode) */
    uint16_t w, h;
    Color color;
    Font font;
    CanvasOrientation orientation;
};

struct ViewPort {
    ViewPortDrawCallback draw;
    void *draw_ctx;
    ViewPortInputCallback input;
    void *input_ctx;
    ViewPortOrientation orientation;
    bool enabled, added;
    struct ViewPort *next;
};

struct Gui {
    struct Canvas canvas;
    struct ViewPort *ports;
    GuiCanvasCommitCallback fb_cb[2];
    void *fb_ctx[2];
    bool direct;
    bool dirty;
    bool full;
    ViewPortOrientation shown;      /* orientation the border was drawn for */
};
static struct Gui gui;

void flipper_gui_reset(void)
{
    memset(&gui, 0, sizeof(gui));
    gui.canvas.w = 128; gui.canvas.h = 64;
    gui.canvas.color = ColorBlack;
    gui.canvas.font = FontSecondary;
    gui.full = true;
}

void *flipper_gui_record(void) { return &gui; }

/* ----------------------------------------------------------------------- */
/* Pixels. */

static inline void put(Canvas *c, int x, int y)
{
    uint8_t *p;
    uint8_t m;
    if ((unsigned)x >= c->w || (unsigned)y >= c->h) return;
    p = &c->buf[(y >> 3) * c->w + x];
    m = 1u << (y & 7);
    switch (c->color) {
    case ColorBlack: *p |= m; break;
    case ColorWhite: *p &= ~m; break;
    default: *p ^= m; break;
    }
}

/* Spans and the shapes below are clipped to the canvas before their loops:
 * sizes come from game maths, and a negative one arrives as a huge size_t
 * (a game that draws a line as a box end - start wide), which would otherwise
 * loop for billions of clipped pixels and hang the player. */
static void hline(Canvas *c, int x0, int x1, int y)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if ((unsigned)y >= c->h) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= (int)c->w) x1 = c->w - 1;
    for (; x0 <= x1; x0++) put(c, x0, y);
}

static void vline(Canvas *c, int x, int y0, int y1)
{
    if ((unsigned)x >= c->w) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= (int)c->h) y1 = c->h - 1;
    for (; y0 <= y1; y0++) put(c, x, y0);
}

/* The rows [*j0, *j1) of a shape h tall at y that fall on a canvas
 * extent n tall. */
static void visible(int32_t y, size_t h, size_t n, size_t *j0, size_t *j1)
{
    int64_t a = y < 0 ? -(int64_t)y : 0, b = (int64_t)n - y;
    if (b > (int64_t)h) b = h;
    *j0 = a;
    *j1 = b > a ? (size_t)b : (size_t)a;
}

void canvas_reset(Canvas *c)
{
    memset(c->buf, 0, CANVAS_BYTES);
    c->color = ColorBlack;
    c->font = FontSecondary;
}
void canvas_clear(Canvas *c) { memset(c->buf, 0, CANVAS_BYTES); }
uint8_t *canvas_get_buffer(Canvas *c) { return c->buf; }
size_t canvas_get_buffer_size(const Canvas *c) { UNUSED(c); return CANVAS_BYTES; }
size_t canvas_width(const Canvas *c) { return c->w; }
size_t canvas_height(const Canvas *c) { return c->h; }
void canvas_set_color(Canvas *c, Color col) { c->color = col; }
void canvas_set_bitmap_mode(Canvas *c, bool alpha) { UNUSED(c); UNUSED(alpha); }   /* icons are always transparent */
void canvas_invert_color(Canvas *c)
{
    if (c->color == ColorBlack) c->color = ColorWhite;
    else if (c->color == ColorWhite) c->color = ColorBlack;
}
void canvas_set_font(Canvas *c, Font f) { c->font = f; }
void canvas_set_font_direction(Canvas *c, CanvasDirection d) { UNUSED(c); UNUSED(d); }

void canvas_draw_dot(Canvas *c, int32_t x, int32_t y) { put(c, x, y); }

static int outcode(const Canvas *c, int32_t x, int32_t y)
{
    return (x < 0) | (x >= c->w) << 1 | (y < 0) << 2 | (y >= c->h) << 3;
}

/* Cohen-Sutherland: clip to the canvas first, so a line with wild
 * coordinates costs no more than one across the screen. Integer only:
 * this file must not need the FPU. */
static bool clip_line(const Canvas *c, int32_t *x0, int32_t *y0,
                      int32_t *x1, int32_t *y1)
{
    int o0 = outcode(c, *x0, *y0), o1 = outcode(c, *x1, *y1);
    while (o0 | o1) {
        int o = o0 ? o0 : o1;
        int64_t dx = (int64_t)*x1 - *x0, dy = (int64_t)*y1 - *y0;
        int64_t x, y;
        if (o0 & o1)
            return false;
        if (o & 1)      { x = 0;        y = *y0 + dy * (x - *x0) / dx; }
        else if (o & 2) { x = c->w - 1; y = *y0 + dy * (x - *x0) / dx; }
        else if (o & 4) { y = 0;        x = *x0 + dx * (y - *y0) / dy; }
        else            { y = c->h - 1; x = *x0 + dx * (y - *y0) / dy; }
        if (o == o0) { *x0 = x; *y0 = y; o0 = outcode(c, *x0, *y0); }
        else         { *x1 = x; *y1 = y; o1 = outcode(c, *x1, *y1); }
    }
    return true;
}

void canvas_draw_line(Canvas *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    if (!clip_line(c, &x0, &y0, &x1, &y1))
        return;
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        put(c, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void canvas_draw_box(Canvas *c, int32_t x, int32_t y, size_t w, size_t h)
{
    size_t j, j0, j1;
    int64_t x1 = (int64_t)x + w - 1;
    if (!w) return;
    if (x1 >= c->w) x1 = c->w - 1;
    visible(y, h, c->h, &j0, &j1);
    for (j = j0; j < j1; j++) hline(c, x, (int)x1, y + (int)j);
}

void canvas_draw_frame(Canvas *c, int32_t x, int32_t y, size_t w, size_t h)
{
    if (!w || !h) return;
    hline(c, x, x + w - 1, y);
    hline(c, x, x + w - 1, y + h - 1);
    vline(c, x, y, y + h - 1);
    vline(c, x + w - 1, y, y + h - 1);
}

static void circle_pts(Canvas *c, int cx, int cy, int x, int y, bool fill)
{
    if (fill) {
        hline(c, cx - x, cx + x, cy + y);
        hline(c, cx - x, cx + x, cy - y);
        hline(c, cx - y, cx + y, cy + x);
        hline(c, cx - y, cx + y, cy - x);
    } else {
        put(c, cx + x, cy + y); put(c, cx - x, cy + y);
        put(c, cx + x, cy - y); put(c, cx - x, cy - y);
        put(c, cx + y, cy + x); put(c, cx - y, cy + x);
        put(c, cx + y, cy - x); put(c, cx - y, cy - x);
    }
}

static void circle(Canvas *c, int cx, int cy, size_t r, bool fill)
{
    int x = 0, y = r, d = 3 - 2 * r;
    if (r > INT16_MAX) return;  /* a negative radius */
    while (y >= x) {
        circle_pts(c, cx, cy, x, y, fill);
        x++;
        if (d > 0) { y--; d += 4 * (x - y) + 10; }
        else d += 4 * x + 6;
    }
}
void canvas_draw_circle(Canvas *c, int32_t x, int32_t y, size_t r) { circle(c, x, y, r, false); }
void canvas_draw_disc(Canvas *c, int32_t x, int32_t y, size_t r) { circle(c, x, y, r, true); }

void canvas_draw_rframe(Canvas *c, int32_t x, int32_t y, size_t w, size_t h, size_t r)
{
    if (w < 2 * r + 1 || h < 2 * r + 1) { canvas_draw_frame(c, x, y, w, h); return; }
    hline(c, x + r, x + w - 1 - r, y);
    hline(c, x + r, x + w - 1 - r, y + h - 1);
    vline(c, x, y + r, y + h - 1 - r);
    vline(c, x + w - 1, y + r, y + h - 1 - r);
    /* corners, one quadrant each */
    {
        int px = 0, py = r, d = 3 - 2 * r;
        while (py >= px) {
            put(c, x + r - px, y + r - py); put(c, x + r - py, y + r - px);
            put(c, x + w - 1 - r + px, y + r - py); put(c, x + w - 1 - r + py, y + r - px);
            put(c, x + r - px, y + h - 1 - r + py); put(c, x + r - py, y + h - 1 - r + px);
            put(c, x + w - 1 - r + px, y + h - 1 - r + py);
            put(c, x + w - 1 - r + py, y + h - 1 - r + px);
            px++;
            if (d > 0) { py--; d += 4 * (px - py) + 10; }
            else d += 4 * px + 6;
        }
    }
}

void canvas_draw_rbox(Canvas *c, int32_t x, int32_t y, size_t w, size_t h, size_t r)
{
    size_t j, j0, j1;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    visible(y, h, c->h, &j0, &j1);
    for (j = j0; j < j1; j++) {
        int inset = 0;
        if (j < r) inset = r - (int)sqrt((double)(r * r - (r - j - 0.5) * (r - j - 0.5)) + 0.5);
        else if (j >= h - r) {
            double dy = j - (h - r) + 0.5;
            inset = r - (int)sqrt((double)(r * r) - dy * dy + 0.5);
        }
        hline(c, x + inset, x + (int)w - 1 - inset, y + (int)j);
    }
}

void canvas_draw_triangle(Canvas *c, int32_t x, int32_t y, size_t base, size_t height,
                          CanvasDirection dir)
{
    /* Filled isosceles triangle: base on the left/top edge, apex opposite. */
    size_t i, i0, i1;
    if (!base || !height) return;
    if (dir == CanvasDirectionLeftToRight) visible(x, height, c->w, &i0, &i1);
    else visible(y, height, c->h, &i0, &i1);
    for (i = i0; i < i1; i++) {
        int len = base - (int)(base * i / height);
        int off = (base - len) / 2;
        if (dir == CanvasDirectionLeftToRight) vline(c, x + i, y + off, y + off + len - 1);
        else hline(c, x + off, x + off + len - 1, y + i);
    }
}

void canvas_draw_xbm(Canvas *c, int32_t x, int32_t y, size_t w, size_t h, const uint8_t *bm)
{
    size_t i, j, stride = (w + 7) / 8;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (bm[j * stride + i / 8] & (1u << (i & 7))) put(c, x + i, y + j);
}

void canvas_draw_icon(Canvas *c, int32_t x, int32_t y, const Icon *icon)
{
    canvas_draw_xbm(c, x, y, icon->width, icon->height, icon->frames[0]);
}

void canvas_draw_icon_ex(Canvas *c, int32_t x, int32_t y, const Icon *icon, IconRotation rot)
{
    int w = icon->width, h = icon->height, stride = (w + 7) / 8;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            if (!(icon->frames[0][j * stride + i / 8] & (1u << (i & 7)))) continue;
            switch (rot) {
            case IconRotation90: put(c, x + h - 1 - j, y + i); break;
            case IconRotation180: put(c, x + w - 1 - i, y + h - 1 - j); break;
            case IconRotation270: put(c, x + j, y + w - 1 - i); break;
            default: put(c, x + i, y + j); break;
            }
        }
    }
}

void elements_button_center(Canvas *c, const char *str)
{
    int w = (int)canvas_string_width(c, str) + 10;
    int x = 64 - w / 2;
    canvas_draw_rframe(c, x, 52, w, 12, 2);
    canvas_draw_str_aligned(c, 64, 62, AlignCenter, AlignBottom, str);
}

/* ----------------------------------------------------------------------- */
/* Text. */

/* 3x5 glyphs for the micro font: digits and a few symbols, rows MSB first. */
static const struct { char ch; uint8_t rows[5]; } micro_glyphs[] = {
    {'0', {7, 5, 5, 5, 7}}, {'1', {2, 6, 2, 2, 7}}, {'2', {7, 1, 7, 4, 7}},
    {'3', {7, 1, 7, 1, 7}}, {'4', {5, 5, 7, 1, 1}}, {'5', {7, 4, 7, 1, 7}},
    {'6', {7, 4, 7, 5, 7}}, {'7', {7, 1, 1, 1, 1}}, {'8', {7, 5, 7, 5, 7}},
    {'9', {7, 5, 7, 1, 7}}, {'-', {0, 0, 7, 0, 0}}, {'+', {0, 2, 7, 2, 0}},
    {'.', {0, 0, 0, 0, 2}}, {',', {0, 0, 0, 2, 4}}, {':', {0, 2, 0, 2, 0}},
    {'/', {1, 1, 2, 4, 4}}, {'*', {0, 5, 2, 5, 0}},
};

void canvas_set_custom_u8g2_font(Canvas *c, const uint8_t *font)
{
    UNUSED(font);
    c->font = FontMicro;
}

static const struct flip_font *font_of(const Canvas *c, int *scale)
{
    *scale = 1;
    switch (c->font) {
    case FontPrimary: return &flip_font_primary;
    case FontKeyboard: return &flip_font_keyboard;
    case FontBigNumbers: *scale = 2; return &flip_font_secondary;
    default: return &flip_font_secondary;
    }
}

static const struct flip_glyph *glyph_of(const struct flip_font *f, unsigned char ch)
{
    if (ch < 32 || ch > 126) ch = '?';
    return &f->glyphs[ch - 32];
}

uint16_t canvas_string_width(Canvas *c, const char *s)
{
    int scale, w = 0;
    if (c->font == FontMicro) return strlen(s) * 4;
    const struct flip_font *f = font_of(c, &scale);
    for (; *s; s++) w += glyph_of(f, *s)->dwidth * scale;
    return w;
}

uint8_t canvas_glyph_width(Canvas *c, uint16_t ch)
{
    int scale;
    const struct flip_font *f = font_of(c, &scale);
    return glyph_of(f, ch)->dwidth * scale;
}

size_t canvas_current_font_height(const Canvas *c)
{
    int scale;
    if (c->font == FontMicro) return 5;
    return font_of(c, &scale)->height * scale;
}

static void draw_micro(Canvas *c, int x, int y, const char *s)
{
    for (; *s; s++, x += 4) {
        unsigned i, r, b;
        for (i = 0; i < ARRAYLEN(micro_glyphs); i++) {
            if (micro_glyphs[i].ch != *s) continue;
            for (r = 0; r < 5; r++)
                for (b = 0; b < 3; b++)
                    if (micro_glyphs[i].rows[r] & (4u >> b)) put(c, x + b, y - 4 + r);
        }
    }
}

void canvas_draw_str(Canvas *c, int32_t x, int32_t y, const char *s)
{
    int scale;
    if (c->font == FontMicro) { draw_micro(c, x, y, s); return; }
    const struct flip_font *f = font_of(c, &scale);
    for (; *s; s++) {
        const struct flip_glyph *g = glyph_of(f, *s);
        int top = y - (g->yoff + g->h) * scale + 1;
        int r, b, sx, sy;
        for (r = 0; r < g->h; r++) {
            uint16_t bits = f->rows[g->row + r];
            for (b = 0; b < g->w; b++) {
                if (!(bits & (0x8000u >> b))) continue;
                for (sy = 0; sy < scale; sy++)
                    for (sx = 0; sx < scale; sx++)
                        put(c, x + (g->xoff + b) * scale + sx, top + r * scale + sy);
            }
        }
        x += g->dwidth * scale;
    }
}

void canvas_draw_str_aligned(Canvas *c, int32_t x, int32_t y, Align h, Align v, const char *s)
{
    int scale, fh = canvas_current_font_height(c);
    const struct flip_font *f = font_of(c, &scale);
    int asc = c->font == FontMicro ? 5 : f->ascent * scale;
    switch (h) {
    case AlignRight: x -= canvas_string_width(c, s); break;
    case AlignCenter: x -= canvas_string_width(c, s) / 2; break;
    default: break;
    }
    switch (v) {
    case AlignTop: y += asc - 1; break;
    case AlignCenter: y += asc - 1 - fh / 2; break;
    default: y -= fh - asc; break;   /* bottom */
    }
    canvas_draw_str(c, x, y, s);
}

/* ----------------------------------------------------------------------- */
/* Presenting. */

static void present(void)
{
    Canvas *c = &gui.canvas;
    int ox = (LCD_WIDTH - c->w) / 2, oy = (LCD_HEIGHT - c->h) / 2;
    int x, y;
    fb_data row[128];

    if (gui.full) {
        hint_strip_draw(flipper_game.hint, oy + c->h);
        rb->lcd_set_background(rb->global_settings->bg_color);
    }
    for (y = 0; y < c->h; y++) {
        const uint8_t *col = &c->buf[(y >> 3) * c->w];
        uint8_t m = 1u << (y & 7);
        if (flipper_game.blend) {
            const uint8_t *pcol = &c->prev[(y >> 3) * c->w];
            for (x = 0; x < c->w; x++) {
                int n = ((col[x] & m) != 0) + ((pcol[x] & m) != 0);
                row[x] = n == 2 ? COL_FG : n == 1 ? COL_MID : COL_BG;
            }
        } else {
            for (x = 0; x < c->w; x++) row[x] = (col[x] & m) ? COL_FG : COL_BG;
        }
        rb->lcd_bitmap(row, ox, oy + y, c->w, 1);
    }
    if (flipper_game.blend) memcpy(c->prev, c->buf, CANVAS_BYTES);
    if (gui.full) {
        rb->lcd_update();
        gui.full = false;
    } else {
        rb->lcd_update_rect(ox, oy, c->w, c->h);
    }
}

static void set_orientation(ViewPortOrientation o)
{
    bool vert = o == ViewPortOrientationVertical || o == ViewPortOrientationVerticalFlip;
    uint16_t w = vert ? 64 : 128, h = vert ? 128 : 64;
    if (gui.canvas.w != w) {
        gui.canvas.w = w; gui.canvas.h = h;
        gui.canvas.orientation = (CanvasOrientation)o;
        gui.full = true;
    }
}

void canvas_commit(Canvas *c)
{
    int i;
    UNUSED(c);
    flipper_checkpoint();
    for (i = 0; i < 2; i++)
        if (gui.fb_cb[i])
            gui.fb_cb[i](gui.canvas.buf, CANVAS_BYTES, gui.canvas.orientation, gui.fb_ctx[i]);
    present();
    /* The game is likely to be running flat out: let the key poll catch up. */
    flipper_pump();
}

void flipper_gui_refresh_all(void)
{
    gui.full = true;
    gui.dirty = true;
    if (gui.direct) present();
}

/* ----------------------------------------------------------------------- */
/* ViewPort and Gui. */

ViewPort *view_port_alloc(void)
{
    ViewPort *v = calloc(1, sizeof(*v));
    if (v) v->enabled = true;
    return v;
}
void view_port_free(ViewPort *v) { free(v); }
void view_port_enabled_set(ViewPort *v, bool e) { v->enabled = e; gui.dirty = true; }
void view_port_draw_callback_set(ViewPort *v, ViewPortDrawCallback cb, void *ctx)
{
    v->draw = cb; v->draw_ctx = ctx;
}
void view_port_input_callback_set(ViewPort *v, ViewPortInputCallback cb, void *ctx)
{
    v->input = cb; v->input_ctx = ctx;
}
void view_port_update(ViewPort *v) { UNUSED(v); gui.dirty = true; }
void view_port_set_orientation(ViewPort *v, ViewPortOrientation o)
{
    v->orientation = o;
    gui.dirty = true;
}

void gui_add_view_port(Gui *g, ViewPort *v, GuiLayer layer)
{
    UNUSED(layer);
    v->next = g->ports;     /* most recently added is on top */
    g->ports = v;
    v->added = true;
    g->dirty = true;
}

void gui_remove_view_port(Gui *g, ViewPort *v)
{
    ViewPort **pp;
    for (pp = &g->ports; *pp; pp = &(*pp)->next)
        if (*pp == v) { *pp = v->next; break; }
    v->added = false;
}

void gui_add_framebuffer_callback(Gui *g, GuiCanvasCommitCallback cb, void *ctx)
{
    int i;
    for (i = 0; i < 2; i++)
        if (!g->fb_cb[i]) { g->fb_cb[i] = cb; g->fb_ctx[i] = ctx; return; }
}

void gui_remove_framebuffer_callback(Gui *g, GuiCanvasCommitCallback cb, void *ctx)
{
    int i;
    for (i = 0; i < 2; i++)
        if (g->fb_cb[i] == cb && g->fb_ctx[i] == ctx) g->fb_cb[i] = NULL;
}

Canvas *gui_direct_draw_acquire(Gui *g)
{
    g->direct = true;
    canvas_reset(&g->canvas);
    return &g->canvas;
}
void gui_direct_draw_release(Gui *g) { g->direct = false; g->dirty = true; }

void flipper_gui_dispatch(const InputEvent *ev)
{
    ViewPort *v;
    for (v = gui.ports; v; v = v->next) {
        if (v->enabled && v->input) {
            InputEvent copy = *ev;
            v->input(&copy, v->input_ctx);
            return;
        }
    }
}

void flipper_gui_redraw(void)
{
    ViewPort *v;
    if (!gui.dirty || gui.direct) return;
    for (v = gui.ports; v; v = v->next) {
        if (v->enabled && v->draw) {
            gui.dirty = false;
            set_orientation(v->orientation);
            canvas_reset(&gui.canvas);
            v->draw(&gui.canvas, v->draw_ctx);
            present();
            return;
        }
    }
}
