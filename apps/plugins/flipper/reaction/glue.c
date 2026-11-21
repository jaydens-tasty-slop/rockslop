/* Rockbox glue for the Flipper Reaction Game (see ../README.rockbox). */
#include "flipper_game.h"

int32_t reaction_main(void *p);

static const char *const help[] = {
    "Test your reaction time.",
    "",
    "Press SELECT to start, wait for the beep and \"Press OK!\", then press SELECT as fast as you can. Your time is shown in milliseconds.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Reaction Game",
    "Copyright (c) 2023 Milk-Cool",
    "github.com/Milk-Cool/fz-reaction-game",
    "License: MIT.",
    "Full text: rocks/games/reaction/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Reaction Game",
    .entry = reaction_main,
    .help = help,
    .credits = credits,
    .hint = "Sel=start/react|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/reaction",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
};
