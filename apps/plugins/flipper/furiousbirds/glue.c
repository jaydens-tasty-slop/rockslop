/* Rockbox glue for bmstr-ru's Furious Birds (see ../README.rockbox). */
#include "flipper_game.h"

int32_t furious_birds_app(void *p);

static const char *const help[] = {
    "Aim the bird at the pigs and catch them all in three attempts.",
    "",
    "UP and DOWN change the angle, SELECT fires. After game over, SELECT",
    "starts again; hold SELECT to go back to the title menu.",
    "Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "Furious Birds",
    "Copyright (c) 2023 Alexander Taran (bmstr-ru).",
    "github.com/bmstr-ru/furious-birds",
    "License: MIT. Full text: rocks/games/furiousbirds/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Furious Birds",
    .entry = furious_birds_app,
    .help = help,
    .credits = credits,
    .hint = "Up/Down=aim  Sel=fire|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/furiousbirds",
    .back = FLIPPER_BACK_PASS_OK,
    .quit = FLIPPER_QUIT_LONG,
    .boost = 1,
};
