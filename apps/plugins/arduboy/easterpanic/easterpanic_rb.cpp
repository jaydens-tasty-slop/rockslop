/* Rockbox game description for Easter Panic. Select = A (confirm), Select+Down = B (options / back). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A puzzle by Pharap for Arduboy game jam 2: program the little "
    "character's moves, then run them.", "",
    "Directions: move the cursor.", "",
    "Select: confirm / pick (A).", "",
    "Select + Down: options / back (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Easter Panic",
    "",
    "Pharap (2017), Arduboy game jam 2.",
    "",
    "https://github.com/Pharap/EasterPanic @ 4a7b724",
    "",
    "License: Apache 2.0. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "easterpanic", "Easter Panic", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=confirm  Sel+Down=back|Hold Sel+Up=menu"
};
