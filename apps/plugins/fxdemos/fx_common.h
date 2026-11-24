/* Small helpers shared by the fxdemos plugins: direct framebuffer drawing,
 * integer trigonometry, 5x7 text. Rockbox port code, released under CC0.
 *
 * Everything draws into fx_fb, the LCD framebuffer itself (set by fx_main.h,
 * or by a host test harness). Colours are packed with FB_RGBPACK so the
 * LCD byte order is hidden. */
#ifndef FX_COMMON_H
#define FX_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define FX_DIRS (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)

#define FX_W LCD_WIDTH
#define FX_H LCD_HEIGHT

static fb_data *fx_fb;

#include "fx_sintab.h"
#include "fx_font5x7.h"

/* Angles are 1024 steps per turn; results are Q15 (-32767..32767). */
static inline int fx_sin(int a) { return fx_sintab[a & 1023]; }
static inline int fx_cos(int a) { return fx_sintab[(a + 256) & 1023]; }

static inline unsigned fx_isqrt(unsigned long v)
{
  unsigned long r = 0, bit = 1UL << 30;
  while (bit > v)
    bit >>= 2;
  while (bit)
  {
    if (v >= r + bit)
    {
      v -= r + bit;
      r = (r >> 1) + bit;
    }
    else
      r >>= 1;
    bit >>= 2;
  }
  return r;
}

static uint32_t fx_seed = 0x2545F491u;

static inline unsigned fx_rand(void)
{
  fx_seed = fx_seed * 1103515245u + 12345u;
  return (fx_seed >> 16) & 0x7fff;
}

/* A host-order RGB565 constant as fb_data. */
#if LCD_PIXELFORMAT == RGB565SWAPPED
#define FX_FROM565(c) ((fb_data)((((c) >> 8) | ((c) << 8)) & 0xffff))
#elif LCD_PIXELFORMAT == RGB565
#define FX_FROM565(c) ((fb_data)(c))
#else
#define FX_FROM565(c) FB_RGBPACK((((c) >> 11) << 3), ((((c) >> 5) & 63) << 2), (((c) & 31) << 3))
#endif

static inline fb_data fx_rgb(int r, int g, int b)
{
  return FB_RGBPACK(r, g, b);
}

/* Scale a colour by k/256 (0..256). */
static inline fb_data fx_scale(fb_data p, int k)
{
  return FB_RGBPACK((FB_UNPACK_RED(p) * k) >> 8, (FB_UNPACK_GREEN(p) * k) >> 8,
                    (FB_UNPACK_BLUE(p) * k) >> 8);
}

/* Blend src over dst, a = 0..256 (256 = src only). */
static inline fb_data fx_blend(fb_data dst, fb_data src, int a)
{
  int ia = 256 - a;
  return FB_RGBPACK((FB_UNPACK_RED(src) * a + FB_UNPACK_RED(dst) * ia) >> 8,
                    (FB_UNPACK_GREEN(src) * a + FB_UNPACK_GREEN(dst) * ia) >> 8,
                    (FB_UNPACK_BLUE(src) * a + FB_UNPACK_BLUE(dst) * ia) >> 8);
}

static inline void fx_clear(fb_data c)
{
  for (int i = 0; i < FX_W * FX_H; i++)
    fx_fb[i] = c;
}

static inline void fx_pixel(int x, int y, fb_data c)
{
  if ((unsigned)x < FX_W && (unsigned)y < FX_H)
    fx_fb[y * FX_W + x] = c;
}

static inline void fx_hline(int x, int y, int len, fb_data c)
{
  if ((unsigned)y >= FX_H)
    return;
  if (x < 0)
  {
    len += x;
    x = 0;
  }
  if (x + len > FX_W)
    len = FX_W - x;
  for (fb_data *p = fx_fb + y * FX_W + x; len > 0; len--)
    *p++ = c;
}

static inline void fx_fill_rect(int x, int y, int w, int h, fb_data c)
{
  for (int j = 0; j < h; j++)
    fx_hline(x, y + j, w, c);
}

static inline void fx_line(int x0, int y0, int x1, int y1, fb_data c)
{
  int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
  int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;)
  {
    fx_pixel(x0, y0, c);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

static inline int fx_text_w(const char *s)
{
  int n = 0;
  while (s[n])
    n++;
  return n * 6;
}

static inline void fx_text(int x, int y, const char *s, fb_data c)
{
  for (; *s; s++, x += 6)
  {
    unsigned char ch = (unsigned char)*s;
    if (ch < 32 || ch > 127)
      ch = '?';
    const uint8_t *g = fx_font5x7[ch - 32];
    for (int col = 0; col < 5; col++)
      for (int row = 0; row < 7; row++)
        if (g[col] & (1 << row))
          fx_pixel(x + col, y + row, c);
  }
}

static inline void fx_text_center(int y, const char *s, fb_data c)
{
  fx_text((FX_W - fx_text_w(s)) / 2, y, s, c);
}

/* Text with a one-pixel drop shadow. */
static inline void fx_text_shadow(int x, int y, const char *s, fb_data c)
{
  fx_text(x + 1, y + 1, s, fx_rgb(0, 0, 0));
  fx_text(x, y, s, c);
}

#endif
