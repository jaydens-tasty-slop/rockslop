/* Rockbox glue for the Flipper Apple Grabber (see ../README.rockbox). */
#include "flipper_game.h"

int32_t apple_grabber_app(void *p);

static const char *const help[] = {
    "Catch the falling apples with your basket on the right edge.",
    "",
    "SELECT starts a round. LEFT / RIGHT move the basket up and down.",
    "Missing an apple loses the round; the apples speed up each wave.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Apple Grabber",
    "Copyright (C) Julio Rodriguez (d7d8)",
    "github.com/d7d8/apple-grabber",
    "License: GPL v3.",
    "Full text: rocks/games/applegrabber/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Apple Grabber",
    .entry = apple_grabber_app,
    .help = help,
    .credits = credits,
    .hint = "L/R=move  Sel=start|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/applegrabber",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
