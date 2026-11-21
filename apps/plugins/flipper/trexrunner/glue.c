/* Rockbox glue for the Flipper T-Rex Runner (see ../README.rockbox). */
#include "flipper_game.h"

int32_t trexrunner_app(void);

static const char *const help[] = {
    "Jump over the cacti as the dinosaur runs faster and faster.",
    "",
    "SELECT or UP jumps. After a crash, SELECT starts again.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "T-Rex Runner",
    "Copyright (C) @Rrycbarm",
    "github.com/Rrycbarm/t-rex-runner",
    "License: GPL v3.",
    "Full text: rocks/games/trexrunner/LICENSE.txt on the card.",
    NULL,
};

static int32_t entry(void *p)
{
    (void)p;
    return trexrunner_app();
}

const struct flipper_game flipper_game = {
    .name = "T-Rex Runner",
    .entry = entry,
    .help = help,
    .credits = credits,
    .hint = "Sel/Up=jump|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/trexrunner",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
    .up_is_ok = 1,
};
