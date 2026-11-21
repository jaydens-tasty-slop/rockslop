/* Rockbox glue for the Flipper Bomberduck (see ../README.rockbox). */
#include "flipper_game.h"

int32_t bomberduck_app(void *p);

static const char *const help[] = {
    "Blow up the boxes, reach the exit and avoid the enemies and your own",
    "blasts.",
    "",
    "Direction keys move. SELECT plants a bomb (two at a time); it also",
    "starts the next level when you are done. Bombs get faster each level.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Bomber Duck",
    "Copyright (c) 2023 leo-need-more-coffee (+ @xMasterX).",
    "github.com/xMasterX/all-the-plugins, base_pack/bomberduck",
    "License: MIT. Full text: rocks/games/bomberduck/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Bomber Duck",
    .entry = bomberduck_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=bomb  Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/bomberduck",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_LONG,
    .boost = 1,
};
