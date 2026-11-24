/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Boundary between the C++ Gamebuino META API (gb.cpp) and the Rockbox
 * plugin glue (gb_glue.c). Nothing here depends on plugin.h so the game
 * code stays free of Rockbox macro collisions. */
#ifndef GB_GLUE_H
#define GB_GLUE_H
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Physical keys (same values as BUTTON_*) plus a derived "SELECT held
 * long" bit, combined in key mapping rules. */
#define GB_P_UP     0x01
#define GB_P_DOWN   0x02
#define GB_P_LEFT   0x04
#define GB_P_RIGHT  0x08
#define GB_P_SELECT 0x10
#define GB_P_LONG   0x20

/* Gamebuino META button bits, in the order of the Button enum. */
#define GB_B_DOWN   (1 << 0)
#define GB_B_LEFT   (1 << 1)
#define GB_B_RIGHT  (1 << 2)
#define GB_B_UP     (1 << 3)
#define GB_B_A      (1 << 4)
#define GB_B_B      (1 << 5)
#define GB_B_MENU   (1 << 6)
#define GB_B_HOME   (1 << 7)

/* One mapping rule: when every key in `phys` is held (and not already
 * used by an earlier rule) the buttons `out` read as pressed. Rules are
 * evaluated in order; the table ends with phys == 0. */
struct gb_rule {
    uint8_t phys;
    uint8_t out;
};

/* How the game screen is put on the 128x160 portrait LCD. */
enum gb_view {
    GB_VIEW_FIT,    /* nearest-neighbour scale to 128 px wide, centred */
    GB_VIEW_ROT90   /* 160x128 turned to 128x160 (player holds it sideways) */
};

enum gb_pixels {
    GB_RGB565,      /* uint16_t per pixel */
    GB_INDEXED      /* uint8_t colour index (0..15) per pixel */
};

/* Per-game description, defined by the game's *_rb.cpp. */
struct gb_game {
    const char *name;           /* file-name stem: save file, settings */
    const char *title;
    const char *const *help;    /* NULL-terminated lines; "" = paragraph break */
    const char *const *credits; /* NULL-terminated lines for About */
    const struct gb_rule *keys; /* NULL: default mapping */
    uint8_t menu_keys;          /* physical keys held to open the menu */
    uint16_t menu_ms;           /* ... for this long */
    uint16_t long_ms;           /* hold time for GB_P_LONG (default 400) */
    uint16_t chord_ms;          /* SELECT-alone delay when SELECT chords exist (default 30) */
    uint8_t view;               /* enum gb_view */
    uint8_t pixels;             /* enum gb_pixels */
    uint16_t width, height;     /* game screen size */
    uint8_t *fb;                /* width*height pixels, owned by the game; NULL
                                   when the game draws straight to the panel */
    const char *hint;           /* controls hint shown below the screen */
};

void setup(void);
void loop(void);
extern const struct gb_game gb_game;

#define GB_SAVE_MAX 4096
extern uint8_t gb_save_data[GB_SAVE_MAX];
void gb_save_load(size_t len);          /* fills gb_save_data when a file fits */
int gb_save_loaded(void);
void gb_save_dirty(void);

unsigned long gb_millis(void);      /* game time: excludes pause time */
uint32_t gb_rand(void);
void gb_srand(uint32_t seed);
void gb_idle(void);                 /* yield; handles events and menu */
uint8_t gb_buttons(void);           /* Gamebuino button mask */
/* indexed: fb holds 4-bit palette indices (else RGB565) */
void gb_present(const uint8_t *fb, const uint16_t *palette, int indexed);
/* Direct rows of w*h byte-swapped RGB565 pixels at game position x,y. */
void gb_blit_rect(int x, int y, int w, int h, const uint16_t *swapped565);
void gb_pump(void);                 /* events, pause menu, quit */
int gb_audio_enabled(void);

/* Sound hooks (gb.cpp): fill mono 16-bit samples, return 0 when finished. */
int gb_sound_fill(int16_t *buf, int frames, int rate);
void *gb_heap_area(size_t *size);   /* free part of the plugin arena */
#define GB_HEAP_MAX (24 * 1024)
/* Read part of a game data file (rocks/games/gamebuino/<game>/<name>);
 * returns the number of bytes read, or -1. */
int gb_data_read(const char *name, uint32_t offset, void *dst, size_t len);
/* Open / read / seek / close a game data file (same place as above);
 * the descriptor is -1 when the file is missing. */
int gb_file_open(const char *name);
int gb_file_read(int fd, void *dst, size_t len);
long gb_file_seek(int fd, long offset, int whence);   /* lseek */
void gb_file_close(int fd);
/* Big scratch memory: the audio buffer (stops Rockbox playback). */
void *gb_big_buffer(size_t *size);
/* Show a message and leave the plugin with an error. */
void gb_fatal(const char *msg) __attribute__((noreturn));
/* Streamed music: unsigned 8-bit mono WAV file, mixed with the FX. The
 * ring buffer (a few KB) is the game's to provide; no buffer, no music. */
void gb_music_buffer(void *buf, size_t size);
int gb_music_play(const char *name, int loop);
void gb_music_stop(void);
int gb_music_playing(void);
void gb_quit(void) __attribute__((noreturn));   /* leave the plugin */
int gb_vformat(char *buf, size_t len, const char *fmt, va_list ap);
void gb_audio_start(void);
void gb_audio_lock(int lock);       /* excludes the mixer callback */

#ifdef __cplusplus
}
#endif
#endif
