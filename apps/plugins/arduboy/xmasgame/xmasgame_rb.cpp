/* Rockbox game description for Santa's Happy Little Packer. SELECT = A (place
 * the piece), SELECT + DOWN = B (change piece), hold SELECT + DOWN for 1 s =
 * A + B (restart the level). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN | AB_P_LONG, AB_B_A | AB_B_B},
    {AB_P_SELECT | AB_P_DOWN,             AB_B_B},
    {AB_P_SELECT,                         AB_B_A},
    {AB_P_UP,                             AB_B_UP},
    {AB_P_DOWN,                           AB_B_DOWN},
    {AB_P_LEFT,                           AB_B_LEFT},
    {AB_P_RIGHT,                          AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Pack Santa's presents: fit the pieces into the board.", "",
    "Directions: move the piece.", "",
    "Select: place the piece.", "",
    "Select + Down: change the piece.", "",
    "Hold Select + Down for 1 s: restart the level.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Santa's Happy Little Packer", "",
    "Erwin Ried.", "",
    "https://github.com/ArduboyCollection/ArduboyXmasGame", "",
    "The \"ho ho ho\" voice sample is not played.", "",
    "License: BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "xmasgame", "Santa's Happy Little Packer", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 1000, 0, 0, NULL, 0,
    "Sel=place  Sel+Down=change|Hold Sel+Down=restart"
};
