/* Rockbox game description for KeyKat IT. Select = B (toggle a machine; A or B both work). */
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
    "Keep at least a quarter of the computers running: walk up to a "
    "broken machine and turn it off and on again.", "",
    "Directions: walk.", "",
    "Select: toggle the machine in front of you (release to act).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "KeyKat IT",
    "",
    "Robert Lowe (pngwen), 2017.",
    "",
    "https://github.com/ArduboyCollection/keykat-it @ 2ddac6f",
    "",
    "License: GPL 3 or later. Effects through Playtune.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "keykatit", "KeyKat IT", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=toggle machine|Hold Sel+Up=menu"
};
