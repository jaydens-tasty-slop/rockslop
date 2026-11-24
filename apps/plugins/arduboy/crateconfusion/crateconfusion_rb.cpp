/* Rockbox game description for Crate Confusion. Only A is used: default mapping, SELECT = A. */
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
    "Collect the fuel and dodge the crates.", "",
    "Left/Right: steer.", "",
    "Select: start, confirm, and thrust (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Crate Confusion",
    "",
    "phoboslab (github.com/phoboslab/arduboy-games).",
    "",
    "https://github.com/phoboslab/arduboy-games",
    "",
    "License: MIT (the LICENSE file names OBONO as copyright holder).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "crateconfusion", "Crate Confusion", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Left/Right=steer  Sel=A|Hold Sel+Up=menu"
};
