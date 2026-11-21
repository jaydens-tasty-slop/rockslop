/* Rockbox game description for Begemmed. B toggles gem-swap mode, A pauses: SELECT = B, SELECT + DOWN = A. */
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
    "A match-three gem puzzle: swap neighbouring gems to line up three "
    "or more.", "",
    "Directions: move the cursor; in swap mode they swap the gem in "
    "that direction.", "",
    "Select: toggle swap mode (B); confirms in menus.", "",
    "Select + Down: pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Begemmed",
    "",
    "TEAM a.r.g.: JO3RI (2015).",
    "",
    "https://github.com/Team-ARG-Museum/ID-30-BEGEMMED",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "begemmed", "Begemmed", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=mode  Sel+Down=pause|Hold Sel+Up=menu"
};
