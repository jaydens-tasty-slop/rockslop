/* Rockbox game description for Frogger (Simon Holmes). Directions only. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Guide the frog across the road and river to the far bank.", "",
    "Directions: hop.  Select (B) and Select + Down (A) are unused.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Frogger", "",
    "Simon Holmes, github.com/filmote/Frogger", "",
    "License: BSD 3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "frogger", "Frogger", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dirs=hop|Sel+Up=menu"
};
