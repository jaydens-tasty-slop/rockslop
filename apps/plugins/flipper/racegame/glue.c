/* Rockbox glue for mrc19056's Race Game (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t race_game_app(void *p);

static const char *const help[] = {
    "Dodge the traffic on a three-lane road and collect coins.",
    "",
    "Left/Right change lane. In the menu, Up/Down choose and SELECT",
    "changes the entry (Start, Sound, Night, Difficulty). Hold SELECT to",
    "pause back to the menu (from the menu it exits). Shields, magnets and",
    "fuel are power-ups. Hold SELECT+UP for half a second to open the",
    "Rockbox menu.",
    NULL,
};

static const char *const credits[] = {
    "Race Game",
    "Copyright (c) 2026 Mehmet Mirac ARIK.",
    "github.com/mrc19056/flipper-race-game",
    "License: MIT. Full text: rocks/games/racegame/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Race Game",
    .entry = race_game_app,
    .help = help,
    .credits = credits,
    .hint = "Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/racegame",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_LONG,
    .boost = 1,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
