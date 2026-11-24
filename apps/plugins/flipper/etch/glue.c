/* Rockbox glue for the Flipper Etch A Sketch (see ../README.rockbox). */
#include "flipper_game.h"

int32_t etch_a_sketch_app(void *p);

static const char *const help[] = {
    "Draw with the cursor on a 64x32 board.",
    "",
    "Arrow keys move the cursor. SELECT toggles the dot under the cursor.",
    "Hold SELECT to start or stop drawing while you move.",
    "Hold DOWN to clear the board.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Etch A Sketch",
    "Copyright (C) @SimplyMinimal",
    "github.com/SimplyMinimal/FlipperZero-Etch-A-Sketch",
    "License: GPL v3.",
    "Full text: rocks/games/etch/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Etch A Sketch",
    .entry = etch_a_sketch_app,
    .help = help,
    .credits = credits,
    .hint = "Hold Sel=draw  Down=clear|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/etch",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
};
