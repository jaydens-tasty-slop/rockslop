/* Rockbox game description for A-Maze. Select = B (menu/pause); directions move; A = Select+Down. */
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
    "A game of randomly generated mazes in several modes: walk, "
    "collect, escape in the dark.", "",
    "Directions: move.", "",
    "Select: menu / pause (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "A-Maze",
    "",
    "Alojz Jakob; Arduboy port by Keyboard_Camper.",
    "",
    "https://github.com/alojzjakob/A-Maze @ 37e2305",
    "",
    "License: GPL 3. Effects through the Playtune shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "amazeab", "A-Maze", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=menu|Hold Sel+Up=menu"
};
