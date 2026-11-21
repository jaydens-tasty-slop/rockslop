/* Rockbox game description for Chicken Cross. Default mapping: SELECT = A, SELECT + DOWN = B. */
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
    "Help the chicken cross the busy roads and rivers without getting "
    "hit.", "",
    "Directions: hop.", "",
    "Select: start, continue, confirm (A).", "",
    "Select + Down: toggle sound on the title screen (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Chicken Cross",
    "",
    "David Martinez (2015); Arduboy2 port by Scott Allen (2016).",
    "",
    "https://github.com/dragula96/ChickenX",
    "",
    "License: LGPL 2.1 or later per the source header (GPL 2 file in "
    "the repository). Sound effects via ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "chickenx", "Chicken Cross", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B (sound)|Hold Sel+Up=menu"
};
