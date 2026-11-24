/* Rockbox game description for New Blocks on the Kid. Select = A (drop and continue); Select+Down = B does the same. */
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
    "Stack falling single blocks in a 10x5 field; completed rows score.", "",
    "Left/Right: move the block.", "",
    "Down: drop.", "",
    "Select: drop and continue (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "New Blocks on the Kid",
    "",
    "bateske (2020), Arduboy Game Jam 5.",
    "",
    "https://github.com/bateske/NewBlocksOnTheKid @ 133fb6a",
    "",
    "License: GPL 3. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "newblocks", "New Blocks on the Kid", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=drop+go  Down=drop|Hold Sel+Up=menu"
};
