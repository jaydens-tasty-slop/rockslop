/* Rockbox glue for Abzac's FlipRogue (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t fliprogue_app(void *p);

static const char *const help[] = {
    "A pocket roguelike: 18 floors, three classes, find the Orb of Yonder.",
    "",
    "Direction keys move and attack. Tap SELECT to rest, use stairs or",
    "open a chest. Hold SELECT (Back) for the menu: inventory, look, help.",
    "In menus SELECT tap confirms, hold SELECT goes back.",
    "Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "FlipRogue",
    "Copyright (C) Abzac.",
    "github.com/Abzac/fliprogue",
    "License: GPL v3. Full text: rocks/games/fliprogue/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "FlipRogue",
    .entry = fliprogue_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=ok  Hold Sel=back|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/fliprogue",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
