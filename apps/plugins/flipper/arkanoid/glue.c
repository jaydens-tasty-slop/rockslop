/* Rockbox glue for the Flipper Arkanoid (see ../README.rockbox). */
#include "flipper_game.h"

int32_t arkanoid_game_app(void *p);

static const char *const help[] = {
    "Break all the bricks with the ball.",
    "",
    "Left/Right move the paddle (hold to keep moving). SELECT releases the",
    "ball. Up/Down change the ball speed. Three lives per game.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Arkanoid",
    "By @gotnull and @xMasterX (Flipper plugin).",
    "github.com/xMasterX/all-the-plugins, base_pack/arkanoid",
    "License: GPL v3 (the repository's; the app file has no own header).",
    "Full text: rocks/games/arkanoid/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Arkanoid",
    .entry = arkanoid_game_app,
    .help = help,
    .credits = credits,
    .hint = "L/R=move  Sel=launch|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/arkanoid",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
