/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Interface between the Flipper compatibility layer and one ported game.
 * Each game's glue file defines `flipper_game`; the layer supplies the
 * plugin entry point, the start menu and the Furi/GUI services. */
#ifndef FLIPPER_GAME_H
#define FLIPPER_GAME_H
#include <stddef.h>
#include <stdint.h>

enum flipper_back {
    FLIPPER_BACK_MENU,      /* Back is never sent by a key hold */
    FLIPPER_BACK_MENU_OK = FLIPPER_BACK_MENU,   /* old names */
    FLIPPER_BACK_MENU_DOWN = FLIPPER_BACK_MENU,
    FLIPPER_BACK_PASS_OK,   /* hold SELECT: virtual Back key sent to the game */
};

enum flipper_quit {
    FLIPPER_QUIT_SHORT,     /* menu Quit sends Back press, release, short */
    FLIPPER_QUIT_LONG,      /* menu Quit holds Back until its long event */
};

struct flipper_game {
    const char *name;                   /* menu title */
    int32_t (*entry)(void *arg);        /* the Flipper app's entry point */
    const char *const *help;            /* text lines, NULL terminated */
    const char *const *credits;         /* about lines, NULL terminated */
    const char *hint;                   /* controls hint under the screen ('|' lines) */
    const char *data_dir;               /* SD directory for the game's files */
    uint8_t back, quit;                 /* enum flipper_back / flipper_quit */
    uint8_t boost;                      /* run with the CPU boosted */
    uint8_t up_is_ok;
    /* SELECT split for games that need two action buttons: a tap sends key
     * sel_tap_key on release, holding sel_long_ms sends sel_long_key once
     * (keys as InputKey + 1, 0 = SELECT is a plain OK). */
    uint8_t sel_tap_key, sel_long_key;
    uint16_t sel_long_ms;
    uint16_t back_up_hold_ms;           /* holding UP this long also holds Back (0: never) */
    uint16_t menu_hold_ms;              /* hold SELECT+UP this long for the pause menu (0: 500 ms) */
    uint8_t up_is_ok_dummy;                   /* UP also acts as OK (SELECT is the power key) */
    uint8_t blend;                      /* show two frames averaged (flicker grey) */
    uint32_t heap_limit;                /* max bytes for malloc (0: all) */
};

extern const struct flipper_game flipper_game;

/* Layer services used by games' glue code. */
const char *flipper_data_path(const char *name);    /* data_dir/name */
#endif
