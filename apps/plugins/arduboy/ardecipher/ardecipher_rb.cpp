/* Rockbox game description for ARDecipher. Select = A (confirm), Select+Down = B (help). */
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
    "Like the Fallout terminal hack: find the password within 5 tries; "
    "each wrong word shows how many letters match.", "",
    "Directions: move the cursor.", "",
    "Select: pick a word (A).", "",
    "Select + Down: help (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "ARDecipher",
    "",
    "databhor (2018).",
    "",
    "https://github.com/databhor/ARDecipher @ 2b2255a",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardecipher", "ARDecipher", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=pick  Sel+Down=help|Hold Sel+Up=menu"
};
