/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Boundary between the C++ Arduboy API and the Rockbox plugin glue
 * (ab_glue.c). Nothing here depends on plugin.h so game code stays free of
 * Rockbox macro collisions. */
#ifndef AB_GLUE_H
#define AB_GLUE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AB_EEPROM_SIZE 1024

/* Physical keys (same values as BUTTON_*) plus a derived "SELECT held long"
 * bit, combined in key mapping rules. */
#define AB_P_UP     0x01
#define AB_P_DOWN   0x02
#define AB_P_LEFT   0x04
#define AB_P_RIGHT  0x08
#define AB_P_SELECT 0x10
#define AB_P_LONG   0x20  /* SELECT held for long_ms */
#define AB_P_TAP    0x40  /* SELECT released before long_ms; lasts tap_ms */

/* Arduboy button bits (Arduboy 1.0 layout, as the games expect). */
#define AB_B_LEFT   (1 << 5)
#define AB_B_RIGHT  (1 << 6)
#define AB_B_UP     (1 << 7)
#define AB_B_DOWN   (1 << 4)
#define AB_B_A      (1 << 3)
#define AB_B_B      (1 << 2)

/* One mapping rule: when every key in `phys` is held (and not already used
 * by an earlier rule) the Arduboy buttons `out` read as pressed. Rules are
 * evaluated in order; the table ends with phys == 0. */
struct ab_rule {
    uint8_t phys;
    uint8_t out;
};

/* Per-game description, defined by the game's *_rb.cpp. */
struct ab_game {
    const char *name;           /* file-name stem: EEPROM, settings */
    const char *title;
    const char *const *help;    /* NULL-terminated lines; "" = paragraph break */
    const char *const *credits; /* NULL-terminated lines for About */
    const struct ab_rule *keys; /* NULL: default mapping */
    uint8_t menu_keys;          /* physical keys held to open the menu */
    uint16_t menu_ms;           /* ... for this long */
    uint16_t long_ms;           /* hold time for AB_P_LONG (default 400) */
    uint16_t tap_ms;            /* how long AB_P_TAP lasts (default 60) */
    uint16_t chord_ms;          /* SELECT-alone delay when SELECT chords exist (default 30) */
    const char *data_file;      /* FX flash data (NULL if none) */
    uint8_t blend;              /* temporal grey: blend with previous frame */
    const char *hint;           /* controls hint shown below the screen */
};

void setup(void);
void loop(void);
extern const struct ab_game ab_game;

extern uint8_t ab_eeprom[AB_EEPROM_SIZE];
void ab_eeprom_dirty(void);
void ab_eeprom_flush(void);

unsigned long ab_millis(void);      /* game time: excludes pause time */
void ab_delay(unsigned long ms);
void ab_srand(unsigned long seed);
unsigned long ab_rand(void);
int rand(void);
void srand(unsigned int seed);
int sprintf(char *buf, const char *fmt, ...);
size_t strnlen(const char *s, size_t n);
void ab_idle(void);
void ab_fatal(const char *msg);      /* shows msg and exits */
uint8_t ab_buttons(void);           /* Arduboy bit mask */
void ab_present(const uint8_t *buf, int invert);
void ab_present_gray(const uint8_t *lo, const uint8_t *hi, int levels);
void ab_set_led(uint8_t r, uint8_t g, uint8_t b);
void ab_pump(void);                 /* events, pause menu, quit */
int ab_audio_enabled(void);
void ab_audio_set(int on);

/* Tone engine hooks (ArduboyTones.cpp) */
int ab_tones_fill(int16_t *buf, int frames, int rate); /* 0 when finished */
int ab_tunes_fill(int16_t *buf, int frames, int rate); /* Playtune, optional */
void ab_audio_start(void);          /* begin streaming ab_tones_fill() */

/* FX data file: the game's flash image, read through a page cache
 * (ab_game.data_file, relative to AB_DIR). Offsets are from the file start. */
void ab_fx_read(uint32_t offset, void *dst, uint32_t len);
uint8_t ab_fx_load_state(uint8_t *state, size_t size);
void ab_fx_save_state(const uint8_t *state, size_t size);

#ifdef __cplusplus
}
#endif
#endif
