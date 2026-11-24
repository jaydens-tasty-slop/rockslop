/* Rockbox glue for the Flipper Pixel RPS (see ../README.rockbox). */
#include "flipper_game.h"

int32_t pixel_rps_app(void *p);

static const char *const help[] = {
    "Rock, paper, scissors: let the hand spin and see what you get.",
    "",
    "SELECT plays another round.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Pixel RPS",
    "Copyright (c) 2026 Ugur",
    "github.com/xantopren/rock-paper-scissors",
    "License: MIT.",
    "Full text: rocks/games/rps/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Pixel RPS",
    .entry = pixel_rps_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=play|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/rps",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
};
