/* Rockbox glue for the Flipper Wave game (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t wave_app(void *p);

static const char *const help[] = {
    "Guide the wave through the tunnel without touching the walls.",
    "",
    "Hold UP to climb, release to dive. SELECT also climbs (briefly).",
    "Hold SELECT to pause; pick a level with UP/DOWN and press UP or SELECT.",
    "Hold SELECT+UP for half a second to open the Rockbox menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Wave",
    "By sergo (Sladkisnovraper)",
    "github.com/Sladkisnovraper/Wave-game-for-Flipper-Zero",
    "License: GPL v3.",
    "Full text: rocks/games/wavegame/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Wave",
    .entry = wave_app,
    .help = help,
    .credits = credits,
    .hint = "Up=climb  Hold Sel=pause|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/wavegame",
    .back = FLIPPER_BACK_PASS_OK,
    .quit = FLIPPER_QUIT_LONG,
    .boost = 1,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
