/* Rockbox glue for Flipper Sudoku (see ../README.rockbox). */
#include "flipper_game.h"

int fzsudoku_app(void *p);

static const char *const help[] = {
    "Fill the 9x9 grid so each row, column and 3x3 box holds 1-9.",
    "",
    "Arrows move the cursor, SELECT changes the digit.",
    "Hold SELECT+UP for half a second to open the Rockbox menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Sudoku",
    "Copyright (c) 2023 @profelis",
    "github.com/xMasterX/all-the-plugins (sudoku)",
    "License: MIT.",
    "Full text: rocks/games/fzsudoku/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Sudoku",
    .entry = (int32_t (*)(void *))fzsudoku_app,
    .help = help,
    .credits = credits,
    .hint = "Arrows=move Sel=digit|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/fzsudoku",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
};
