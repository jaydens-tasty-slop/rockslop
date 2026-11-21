/* Rockbox game description for City Life. Select = A (confirm), Select+Down = B (back). */
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
    "A little life simulation: earn money, shop, play the lottery and "
    "keep the customers happy.", "",
    "Directions: move the cursor / character.", "",
    "Select: confirm (A).", "",
    "Select + Down: back (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "City Life",
    "",
    "ArdFlamingo (2022); Font4x6 by Filmote.",
    "",
    "https://github.com/filmote/Arduboy-City-Life @ 5620fef",
    "",
    "License: MIT; the font is BSD-3-Clause. Effects through "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "citylife", "City Life", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=ok|Hold Sel+Up=menu"
};
