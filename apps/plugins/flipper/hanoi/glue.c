/* Rockbox glue for the Flipper Hanoi Towers (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t flipper_game_hanoi_towers(void *p);

static const char *const help[] = {
    "Move the whole stack of disks to the right-hand peg, one disk at a time, never putting a larger disk on a smaller one.",
    "",
    "LEFT / RIGHT move the cursor, SELECT picks up and puts down a disk.",
    "Hold SELECT for half a second: back to the game's menu.",
    "Hold SELECT+UP for half a second to open the Rockbox menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Hanoi Towers",
    "Copyright (c) 2023 Alexander Taran",
    "github.com/AlexTaran/flipperzero",
    "License: MIT.",
    "Full text: rocks/games/hanoi/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Hanoi Towers",
    .entry = flipper_game_hanoi_towers,
    .help = help,
    .credits = credits,
    .hint = "Sel=pick  Hold Sel=back|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/hanoi",
    .back = FLIPPER_BACK_PASS_OK,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
