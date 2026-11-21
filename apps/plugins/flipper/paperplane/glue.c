/* Rockbox glue for the Flipper Paper Plane (see ../README.rockbox). */
#include "flipper_game.h"

int32_t paperplane_app(void);

static const char *const help[] = {
    "Steer a paper plane down a scrolling canyon without hitting the walls.",
    "",
    "LEFT / RIGHT turns the plane. After a crash, SELECT starts again.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Paper Plane",
    "Copyright (C) @Larry-the-Pig",
    "github.com/Larry-the-Pig/flipper-plane",
    "License: GPL v3.",
    "Full text: rocks/games/paperplane/LICENSE.txt on the card.",
    NULL,
};

static int32_t entry(void *p)
{
    (void)p;
    return paperplane_app();
}

const struct flipper_game flipper_game = {
    .name = "Paper Plane",
    .entry = entry,
    .help = help,
    .credits = credits,
    .hint = "L/R=turn  Sel=restart|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/paperplane",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
