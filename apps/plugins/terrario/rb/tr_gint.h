/* Rockbox platform layer for Terrario. Terrario was written for Casio
 * calculators on the gint library; the sources include this header where
 * they used <gint/...>. It supplies the small part of gint's display, key,
 * heap and math API the game needs, on a 128x64 four-grey virtual screen
 * that is drawn into the middle of the 128x160 colour LCD. */
#pragma once

#include "plugin.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define GUNUSED __attribute__((unused))
#define GYRAM
#define GXRAM

#undef min
#undef max
#undef swap
#define min(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define max(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#define sgn(x) ({ __typeof__(x) _s = (x); _s < 0 ? -1 : (_s > 0 ? 1 : 0); })
#define swap(a, b) do { __typeof__(a) _t = (a); (a) = (b); (b) = _t; } while (0)

/* heap: a small first-fit allocator over a static pool */
void *tr_malloc(size_t n);
void *tr_realloc(void *p, size_t n);
void tr_free(void *p);
#define malloc tr_malloc
#define realloc tr_realloc
#define free tr_free

int tr_rand(void);
void tr_srand(unsigned seed);
#undef rand
#undef srand
#define rand tr_rand
#define srand tr_srand
#undef abs
#define abs(x) ({ int _x = (x); _x < 0 ? -_x : _x; })

#define memset rb->memset
#define memcpy rb->memcpy
#define strlen rb->strlen
#define strncpy rb->strncpy
#define strcmp rb->strcmp

/* ---- display ---- */

#define TR_W 128
#define TR_H 64

typedef int color_t;
enum { C_WHITE = 0, C_LIGHT = 1, C_DARK = 2, C_BLACK = 3, C_NONE = -1 };
enum { DIMAGE_NONE = 0, DIMAGE_NOCLIP = 1 };
enum { DTEXT_LEFT = 0, DTEXT_CENTER = 1, DTEXT_RIGHT = 2 };
enum { DTEXT_TOP = 0, DTEXT_MIDDLE = 1, DTEXT_BOTTOM = 2 };
enum { DGRAY_OFF, DGRAY_ON };

/* A sprite sheet in terrario.dat: 2 bit grey plane, then (if mask) a 1 bit
 * transparency plane; see maketex.py. The pixels are read on first use. */
typedef struct
{
  uint16_t w, h;
  uint8_t mask;
  uint32_t off, len;
  const uint8_t *data;
} bopti_image_t;
typedef struct { int dummy; } font_t;

extern bopti_image_t *const tr_all_images[];
extern const int tr_num_images;

void dclear(int color);
void dupdate(void);
void dpixel(int x, int y, int color);
void dline(int x1, int y1, int x2, int y2, int color);
void drect(int x1, int y1, int x2, int y2, int color);
void drect_border(int x1, int y1, int x2, int y2, int fill, int bw, int bc);
void dtext(int x, int y, int fg, const char *s);
void dtext_opt(int x, int y, int fg, int bg, int ha, int va, const char *s);
void dprint(int x, int y, int fg, const char *fmt, ...);
void dprint_opt(int x, int y, int fg, int bg, int ha, int va, const char *fmt, ...);
void dsize(const char *s, const font_t *f, int *w, int *h);
void dimage(int x, int y, bopti_image_t *img);
void dsubimage(int x, int y, bopti_image_t *img, int left, int top, int w, int h, int flags);
#define dgray(x) ((void)0)
#define dfont(f) ((void)0)

/* ---- keys ---- */

enum { KEYEV_NONE, KEYEV_DOWN, KEYEV_UP, KEYEV_HOLD };
enum
{
  KEY_NONE, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
  KEY_EXE, KEY_EXIT, KEY_MENU, KEY_SHIFT, KEY_ALPHA, KEY_OPTN, KEY_DEL, KEY_ACON, KEY_TAN,
  KEY_DOT, KEY_XOT, KEY_EXP, KEY_2, KEY_4, KEY_6, KEY_7, KEY_8, KEY_9
};
enum { GETKEY_NONE, GETKEY_REP_ALL, GETKEY_REP_ARROWS, GETKEY_REP_PROFILE };
typedef struct { int type; int key; } key_event_t;

key_event_t pollevent(void);
key_event_t getkey_opt(int opts, volatile int *timeout);
#define getkey() getkey_opt(GETKEY_NONE, NULL)
bool keydown(int key);
bool keydown_any(int key, ...);
bool keydown_all(int key, ...);
#define keycode_function(k) ((k) - KEY_F1 + 1)
#define getkey_repeat(a, b) ((void)0)
#define getkey_set_repeat_profile(p) ((void)0)
#define clearevents() tr_input_reset()

/* Input state; tr_input_frame() must run once per game frame. */
void tr_input_frame(void);
void tr_input_reset(void);
extern bool tr_usb; /* USB was connected: leave the game */

/* Screen colours (RGB565 plain) for the four greys, and the LCD y offset */
void tr_set_palette(int which);
int tr_palette_count(void);
const char *tr_palette_name(int which);

/* Sprite data (terrario.dat) */
bool tr_sprites_open(const char *path);
void tr_sprites_close(void);

/* Free memory fragments, for the report */
int tr_heap_used(void);
#include "tr_math.h"
