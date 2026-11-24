/* Rockbox glue for Mentoster's Flipper Hero (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t flipper_hero_app(void *p);

static const char *const help[] = {
    "Enter the stratagem codes as fast as you can (a Helldivers-style",
    "arrow-sequence game).",
    "",
    "Press the direction keys in the order shown. A wrong key resets the",
    "code. Tap SELECT to start or restart, or to leave the pause menu; hold",
    "SELECT to pause (there Left/Right change the settings, including",
    "sound). Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "Flipper Hero",
    "Copyright (c) 2024 Dmitriy Makarov (Mentoster).",
    "github.com/xMasterX/all-the-plugins, non_catalog_apps/flipper_hero",
    "License: MIT. Full text: rocks/games/flipperhero/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Flipper Hero",
    .entry = flipper_hero_app,
    .help = help,
    .credits = credits,
    .hint = "Dirs=code  Sel=start|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/flipperhero",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_LONG,
    .boost = 1,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
