/* Rockbox game description for Tiny 2048. Select = A+B (reset). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT,             AB_B_A | AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A 2048 clone with a twist: 2+2 makes 3, 3+3 makes 4 and so on; the "
    "goal is 10.", "",
    "Directions: slide the tiles.", "",
    "Select: reset the game (A+B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiny 2048",
    "",
    "akkera102 (2017).",
    "",
    "https://github.com/akkera102/03_tiny_2048 @ 4ea9c6b",
    "",
    "License: GPL 2 (the repository carries the GPL text). No sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tiny2048", "Tiny 2048", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=reset|Hold Sel+Up=menu"
};
