/* Rockbox glue for the Flipper Drifter (see ../README.rockbox). */
#include "flipper_game.h"

int32_t drifter_app(void *p);

static const char *const help[] = {
    "Drift a boat down a winding river without touching the banks.",
    "",
    "LEFT / RIGHT: push the boat; the longer you hold, the stronger.",
    "DOWN: switch between a moving boat and moving banks.",
    "SELECT restarts after a crash.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Drifter",
    "Copyright (C) Jed Lejosne",
    "github.com/jean-edouard/flipperzero-drifter",
    "License: GPL v3.",
    "Full text: rocks/games/drifter/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Drifter",
    .entry = drifter_app,
    .help = help,
    .credits = credits,
    .hint = "L/R=steer  Down=mode|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/drifter",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
