/* Rockbox glue for SimplyMinimal's Simon Says (see ../README.rockbox). */
#include "flipper_game.h"

int32_t simon_says_app_entry(void *p);

static const char *const help[] = {
    "Watch the sequence of shapes and tones, then repeat it with the",
    "direction keys. Each round adds a step; mistakes cost a life.",
    "",
    "SELECT starts a game. Hold SELECT+UP for half a second to open this",
    "menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Simon Says",
    "By @SimplyMinimal and @ShehabAttia96.",
    "github.com/xMasterX/all-the-plugins, non_catalog_apps/SimonSays",
    "License: GPL v3. Full text: rocks/games/simonsays/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Simon Says",
    .entry = simon_says_app_entry,
    .help = help,
    .credits = credits,
    .hint = "Dirs=repeat  Sel=start|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/simonsays",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_LONG,
};
