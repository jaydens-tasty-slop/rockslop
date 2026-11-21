/* Rockbox glue for the Flipper Flight Assault (see ../README.rockbox). */
#include "flipper_game.h"

int32_t flight_assault(void *p);

static const char *const help[] = {
    "Shoot the enemy ships coming from the right.",
    "",
    "Arrow keys move the ship, SELECT fires.",
    "Collect shields and extra lives. The game ends when all lives are gone.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Flight Assault",
    "Copyright (C) @evillero",
    "github.com/evillero/flight_assault",
    "License: GPL v3.",
    "Full text: rocks/games/flightassault/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Flight Assault",
    .entry = flight_assault,
    .help = help,
    .credits = credits,
    .hint = "Arrows=move  Sel=fire|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/flightassault",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
