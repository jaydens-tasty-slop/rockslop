/* Rockbox game description for Space Wads. Select = A (fire), Select+Down = B. */
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
    "A small space shooter: dodge asteroids and shoot the wads.", "",
    "Directions: fly.", "",
    "Select: fire / start (A).", "",
    "Select + Down: B.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Space Wads",
    "",
    "Cuddles04 (2023).",
    "",
    "https://github.com/Cuddles04/Space-Wads @ d4b12a2",
    "",
    "License: Apache 2.0. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "spacewads", "Space Wads", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=B|Hold Sel+Up=menu"
};
