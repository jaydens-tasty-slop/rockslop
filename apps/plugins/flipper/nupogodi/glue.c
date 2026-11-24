/* Rockbox glue for sionyx's Nu pogodi (see ../README.rockbox). */
#include "flipper_game.h"

int32_t nupogodi_app(void *p);

static const char *const help[] = {
    "Catch the eggs the chickens drop (Elektronika IM-02, Nu, pogodi!).",
    "",
    "Up/Down and Left/Right put the wolf's basket at one of the four",
    "corners; an egg is caught when it reaches your corner. Three broken",
    "eggs end the game. SELECT switches the sound on and off, and starts",
    "a new game after game over.",
    "Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "Nu pogodi",
    "Copyright (c) 2023 Vadim Balashov (sionyx).",
    "github.com/sionyx/flipper_nupogodi",
    "License: MIT. Full text: rocks/games/nupogodi/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Nu pogodi",
    .entry = nupogodi_app,
    .help = help,
    .credits = credits,
    .hint = "Dirs=basket  Sel=sound|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/nupogodi",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_LONG,
};
