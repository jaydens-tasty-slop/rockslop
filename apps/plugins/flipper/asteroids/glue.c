/* Rockbox glue for antirez's Flipper Asteroids (see README.rockbox). */
#include "flipper_game.h"

int32_t asteroids_app_entry(void *p);

static const char *const help[] = {
    "Destroy the asteroids and do not get hit.",
    "",
    "LEFT / RIGHT turn the ship.",
    "SELECT or UP: tap fires, hold thrusts.",
    "DOWN brakes.",
    "Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "Flipper Asteroids",
    "Copyright (C) 2022-2023 Salvatore Sanfilippo (antirez).",
    "github.com/antirez/flipper-asteroids",
    "License: BSD 2-Clause.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Asteroids",
    .entry = asteroids_app_entry,
    .help = help,
    .credits = credits,
    .hint = "L/R=turn  Sel/Up=fire|Hold Sel/Up=thrust|Down=brake|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/asteroids",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 0,
    .up_is_ok = 1,
    .heap_limit = 0,
};
