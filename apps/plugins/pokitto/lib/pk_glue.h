/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Boundary between the C++ PokittoLib compatibility layer and the Rockbox
 * plugin glue (pk_glue.c). Nothing here depends on plugin.h. */
#ifndef PK_GLUE_H
#define PK_GLUE_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Physical keys (same values as BUTTON_*). */
#define PK_P_UP     0x01
#define PK_P_DOWN   0x02
#define PK_P_LEFT   0x04
#define PK_P_RIGHT  0x08
#define PK_P_SELECT 0x10

/* Pokitto buttons as bit numbers (BTN_* ids of PokittoLib). */
#define PK_B_LEFT   (1 << 0)
#define PK_B_UP     (1 << 1)
#define PK_B_RIGHT  (1 << 2)
#define PK_B_DOWN   (1 << 3)
#define PK_B_A      (1 << 4)
#define PK_B_B      (1 << 5)
#define PK_B_C      (1 << 6)

/* When every key in `phys` is held (and none already used by an earlier
 * rule) the Pokitto buttons `out` read as pressed. Table ends with phys 0. */
struct pk_rule {
    uint8_t phys;
    uint8_t out;
};

/* A key mapping. Without tap/hold, SELECT is whatever `rules` say. With
 * `tap` or `hold`, a lone SELECT is not reported while it is down: its
 * release within `hold_ms` pulses the buttons `tap`, and keeping it down
 * for `hold_ms` pulses `hold` (once). SELECT chords still use `rules`, and
 * do not tap. */
struct pk_keys {
    const struct pk_rule *rules;
    uint8_t tap, hold;
    uint16_t hold_ms;           /* 0: 450 */
};

/* Per-game description, defined by the game's *_rb.cpp. */
struct pk_game {
    const char *name;           /* data dir and save file stem */
    const char *title;
    const char *const *help;    /* NULL-terminated lines; "" = paragraph */
    const char *const *credits; /* NULL-terminated lines for About */
    const struct pk_keys *(*keys)(void); /* current mapping; NULL: default */
    uint8_t menu_keys;          /* physical keys held to open the menu */
    uint16_t menu_ms;           /* ... for this long */
    uint8_t hold_select;        /* SELECT is held in play: drop SYS_POWEROFF */
    const char *hint;           /* controls hint under the screen */
};
extern const struct pk_game pk_game;

int pk_main(void);              /* the game's main() */

#define PK_HZ 100
long pk_ticks(void);            /* game time in 1/PK_HZ s, excludes menus */
void pk_idle(void);             /* sleep one tick, pump events and audio */
int  pk_pump(void);             /* events; 1 if the pause menu ran */
uint8_t pk_buttons(void);       /* PK_B_* mask */

uint16_t pk_rgb(int r, int g, int b);
void pk_viewport(int w, int h); /* output size on the LCD (centred) */
void pk_blit(const uint16_t *px, int y, int w, int h);
void pk_flush(void);

/* Files below the game's data dir (.rockbox/rocks/games/<name>/). */
int pk_file_exists(const char *rel);
int pk_dir_open(const char *rel);               /* 1 if the directory exists */
int pk_dir_next(char *name, size_t n);          /* next sub-directory name, 0 at end */
int pk_file_open(const char *rel);              /* fd or -1 */
int pk_file_create(const char *rel, int truncate);   /* read-write fd or -1 */
int pk_file_write(int fd, const void *buf, size_t n);
int pk_file_read(int fd, void *buf, size_t n);
void pk_file_seek(int fd, long pos);
void pk_file_close(int fd);
int pk_file_load(const char *rel, void *buf, size_t size);
void pk_file_save(const char *rel, const void *buf, size_t size);

/* Reads a data file into the audio buffer (this stops Rockbox's own
 * playback) and returns it; the plugin exits with a message if the file is
 * missing or does not fit. */
const uint8_t *pk_assets_load(const char *rel);

#define PK_VOICES 3
int  pk_voice_play(int voice, const char *rel, int rate, int loop, int vol);
void pk_tone(int note, int wave, int vol, int ms);
void pk_tone_stop(void);
void pk_voice_stop(int voice);
void pk_voice_loop(int voice, int loop);
void pk_voice_pause(int voice, int paused);
int  pk_voice_active(int voice);
void pk_audio_poll(void);

#ifdef __cplusplus
}
#endif
#endif
