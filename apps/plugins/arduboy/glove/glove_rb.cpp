/* Rockbox game description for Glove. Select = B (fire, game's 'A'), Select+Down = A. */
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
    "A Gauntlet style action adventure: clear rooms, gather treasure "
    "and reach the exit.", "",
    "Directions: move.", "",
    "Select: fire the blaster; confirm in menus.", "",
    "Select + Down: A button (secondary).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Glove",
    "",
    "fuopy (2016).",
    "",
    "https://github.com/ArduboyCollection/glove @ 81e9988",
    "",
    "License: MIT. Effects through the v1 tunes shim over ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "glove", "Glove", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=A|Hold Sel+Up=menu"
};
