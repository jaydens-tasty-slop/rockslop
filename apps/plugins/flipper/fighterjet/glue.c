/* Rockbox glue for Flipper Fighter Jet (see ../README.rockbox). */
#include "flipper_game.h"

int fighterjet_app(void *p);

static const char *const help[] = {
    "Dogfight: fly the jet, shoot down every enemy plane in each round.",
    "",
    "LEFT / RIGHT turn. SELECT fires. DOWN brakes.",
    "Hold SELECT+UP for half a second to open the Rockbox menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Fighter Jet",
    "Copyright (c) 2025 Eero Prittinen",
    "github.com/Erbonator3000/flipper-fighter-jet",
    "License: MIT.",
    "Full text: rocks/games/fighterjet/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Fighter Jet",
    .entry = (int32_t (*)(void *))fighterjet_app,
    .help = help,
    .credits = credits,
    .hint = "L/R=turn Sel=fire Dn=brake|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/fighterjet",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
};
