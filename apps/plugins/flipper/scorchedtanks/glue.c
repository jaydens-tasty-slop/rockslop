/* Rockbox glue for the Flipper Scorched Tanks (see ../README.rockbox). */
#include "flipper_game.h"

int32_t scorched_tanks_game_app(void *p);

static const char *const help[] = {
    "Artillery duel against the computer tank.",
    "",
    "UP / DOWN: barrel angle.",
    "LEFT / RIGHT: shot power.",
    "SELECT: fire. The wind pushes the shell.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Scorched Tanks",
    "Copyright (C) @jasniec",
    "github.com/jasniec/flipper-scorched-tanks-game",
    "License: GPL v3.",
    "Full text: rocks/games/scorchedtanks/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Scorched Tanks",
    .entry = scorched_tanks_game_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=fire L/R=power|U/D=angle Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/scorchedtanks",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
