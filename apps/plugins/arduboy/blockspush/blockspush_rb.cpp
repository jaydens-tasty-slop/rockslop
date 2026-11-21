/* Rockbox game description for Blocks. Select = A (start; ends the game while playing), Select+Down = B (reset the level). */
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
    "Push the boxes onto their targets: 31 levels, progress saved.", "",
    "Directions: move and push.", "",
    "Select: start (A); while playing it ends the game.", "",
    "Select + Down: reset the level (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Blocks",
    "",
    "William Woody (2016).",
    "",
    "https://github.com/ArduboyCollection/blocks @ 26b650e",
    "",
    "License: GPL 3 or later. Effects through the v1 tunes shim over "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "blockspush", "Blocks", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=start/quit|Sel+Down=reset|Hold Sel+Up=menu"
};
