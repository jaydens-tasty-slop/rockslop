/* Rockbox glue for FlipperMysticBalloon (see README.rockbox). */
#include "flipper_game.h"

int32_t mybl_app(void *p);

static const char *const help[] = {
    "Guide the kid through 39 levels with balloons. Collect the coins,",
    "reach the door and avoid the traps.",
    "",
    "LEFT / RIGHT walk and steer.",
    "UP looks up and enters the door; hold UP for a moment to suck in",
    "coins and enemies.",
    "DOWN looks down.",
    "SELECT jumps; hold it in the air to float on a balloon.",
    "Hold SELECT+UP for half a second to open this menu (so UP does not",
    "suck in while SELECT is held).",
    NULL,
};

static const char *const credits[] = {
    "Mystic Balloon",
    "Copyright 2016 TEAM a.r.g. (original Arduboy game).",
    "Flipper Zero port: apfxtech, 2025.",
    "github.com/apfxtech/FlipperMysticBalloon",
    "License: MIT. Full text: rocks/games/mystic/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Mystic Balloon",
    .entry = mybl_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=jump  hold=float|Up=door  hold=suck|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/mystic",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
    .back_up_hold_ms = 250,
    .menu_hold_ms = 500,
};
