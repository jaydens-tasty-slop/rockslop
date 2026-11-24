/* Rockbox glue for the Flipper IRONWYRM (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t ironwyrm_app(void *p);

static const char *const help[] = {
    "A turn-based mech fighting game: attach modular plates to your WYRM and survive as long as you can.",
    "",
    "Arrow keys choose, SELECT confirms.",
    "Hold SELECT for half a second to cancel or go back; on the main screen it quits the game.",
    "Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "IRONWYRM",
    "Copyright (c) 2026 Henry Gurney",
    "github.com/Atompunk78/IRONWYRM",
    "License: MIT.",
    "Full text: rocks/games/ironwyrm/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "IRONWYRM",
    .entry = ironwyrm_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=ok  Hold Sel=cancel|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/ironwyrm",
    .back = FLIPPER_BACK_PASS_OK,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
