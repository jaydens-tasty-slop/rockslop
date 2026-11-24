/* Rockbox glue for the Flipper Connect Wires (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t flipper_game_connect_wires(void *p);

static const char *const help[] = {
    "Rotate the tiles so that all wires get connected to the power source.",
    "",
    "Arrow keys move the cursor, SELECT rotates the tile under it.",
    "Hold DOWN (or hold SELECT in the game) to flip the rotation direction.",
    "Hold SELECT for half a second: back to the game's menu.",
    "Hold SELECT+UP for half a second to open the Rockbox menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Connect Wires",
    "Copyright (c) 2023 Alexander Taran",
    "github.com/AlexTaran/flipperzero",
    "License: MIT.",
    "Full text: rocks/games/wires/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Connect Wires",
    .entry = flipper_game_connect_wires,
    .help = help,
    .credits = credits,
    .hint = "Sel=turn Hold Down=flip|Hold Sel=back,Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/wires",
    .back = FLIPPER_BACK_PASS_OK,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
