/* Rockbox game description for Call of Dungeon. Select = A (enter / act), Select+Down = B unused. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Wander a dungeon: walk through doors, meet monsters, pick up "
    "items.", "",
    "Directions: walk; Up: use a door.", "",
    "Select: start / act (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Call of Dungeon",
    "",
    "tako2 (2016).",
    "",
    "https://github.com/tako2/dungeon @ 473dce3",
    "",
    "License: MIT. Effects through the v1 tunes interface. Unfinished "
    "upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "calldungeon", "Call of Dungeon", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=act  Up=door|Hold Sel+Up=menu"
};
