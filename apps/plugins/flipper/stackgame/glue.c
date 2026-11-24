/* Rockbox glue for bergr22's 3D Stack (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t stack_app(void *p);

static const char *const help[] = {
    "Stack the blocks as high as you can.",
    "",
    "A block slides over the tower; tap SELECT to drop it. The overhang is",
    "cut off, a perfect drop keeps the full size. In menus, Up/Down choose",
    "and SELECT picks; hold SELECT to go back (during play: to the title).",
    "Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "3D Stack",
    "Copyright (c) 2026 bergrfpv (bergr22).",
    "github.com/xMasterX/all-the-plugins, non_catalog_apps/stack_game",
    "License: MIT. Full text: rocks/games/stackgame/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "3D Stack",
    .entry = stack_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=drop  Hold Sel=back|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/stackgame",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
