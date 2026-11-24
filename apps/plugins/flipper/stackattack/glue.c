/* Rockbox glue for Negenii's Stack Attack (see ../README.rockbox). */
#include "flipper_game.h"

int32_t stack_attack_app(void *p);

static const char *const help[] = {
    "Push crates, fill rows, don't get crushed.",
    "",
    "Left/Right walk and push a crate; SELECT or UP jumps. A full row",
    "clears and brings another crane. Holding SELECT pauses (Continue,",
    "Options, Menu); in Options SELECT switches a setting, hold SELECT to",
    "go back. Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "Stack Attack",
    "Copyright (c) 2026 Evgeny Ugreninov.",
    "github.com/Negenii/flipper-stack-attack",
    "License: MIT. Full text: rocks/games/stackattack/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Stack Attack",
    .entry = stack_attack_app,
    .help = help,
    .credits = credits,
    .hint = "Sel/Up=jump|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/stackattack",
    .back = FLIPPER_BACK_PASS_OK,
    .quit = FLIPPER_QUIT_LONG,
    .boost = 1,
    .up_is_ok = 1,
};
