/* Rockbox glue for jlaughter's Oregon Trail (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t oregon_trail_app(void *p);

static const char *const help[] = {
    "Travel the Oregon Trail: keep your party alive for 2000 miles.",
    "",
    "Direction keys choose and act (on the trail: Right hunts, Down shows",
    "the map, Left rests). Tap SELECT to confirm or travel a day; hold",
    "SELECT to go back or open the exit prompt. Hold SELECT+UP for half a",
    "second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Oregon Trail",
    "Copyright (c) 2026 jlaughter.",
    "github.com/xMasterX/all-the-plugins, non_catalog_apps/oregon_trail",
    "License: MIT. Full text: rocks/games/oregontrail/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Oregon Trail",
    .entry = oregon_trail_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=ok  Hold Sel=back|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/oregontrail",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
