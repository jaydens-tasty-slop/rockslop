/* Rockbox glue for the Flipper Bounce (see ../README.rockbox). */
#include "flipper_game.h"

int32_t bounce_app(void *p);

static const char *const help[] = {
    "Roll the ball, jump, collect every ring to open the exit.",
    "Avoid thorns and spikers.",
    "",
    "LEFT / RIGHT roll, UP or SELECT jump. SELECT starts and confirms.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Bounce (Ring Bounce)",
    "Copyright (c) 2026 Tom Varghese",
    "github.com/tomxposed/flipper-bounce",
    "License: MIT.",
    "Full text: rocks/games/ringbounce/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Ring Bounce",
    .entry = bounce_app,
    .help = help,
    .credits = credits,
    .hint = "L/R=roll  Up/Sel=jump|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/ringbounce",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
