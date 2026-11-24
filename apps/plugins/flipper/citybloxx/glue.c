/* Rockbox glue for Milk-Cool's City Bloxx (see ../README.rockbox). */
#include "flipper_game.h"

int32_t citybloxx_main(void *p);

static const char *const help[] = {
    "Build the tallest tower you can.",
    "",
    "The crane swings a house block over the tower. SELECT drops it;",
    "land it on top of the last block. Miss by more than a few pixels and",
    "the game is over; SELECT starts a new tower.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "City Bloxx",
    "Copyright (c) 2023 Milk-Cool.",
    "github.com/Milk-Cool/fz-citybloxx",
    "License: MIT. Full text: rocks/games/citybloxx/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "City Bloxx",
    .entry = citybloxx_main,
    .help = help,
    .credits = credits,
    .hint = "Sel=drop  Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/citybloxx",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_LONG,
    .boost = 1,
};
