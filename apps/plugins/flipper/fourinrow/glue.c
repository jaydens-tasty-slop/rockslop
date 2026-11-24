/* Rockbox glue for the Flipper Four in a Row (see ../README.rockbox). */
#include "flipper_game.h"

int32_t four_in_row_app(void *p);

static const char *const help[] = {
    "Connect Four for two players on one device, X and O taking turns.",
    "",
    "LEFT / RIGHT choose the column, SELECT drops the disc.",
    "Get four in a row, column or diagonal to win the round.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Four in Row",
    "By leo-need-more-coffee (Flipper plugin).",
    "github.com/xMasterX/all-the-plugins, apps_source_code/4inrow_game",
    "License: GPL v3 (the repository's).",
    "Full text: rocks/games/fourinrow/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Four in Row",
    .entry = four_in_row_app,
    .help = help,
    .credits = credits,
    .hint = "L/R=column  Sel=drop|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/fourinrow",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
};
