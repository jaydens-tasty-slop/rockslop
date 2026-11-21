/* Rockbox game description for Nielk. Select = A; Select+Down = B. */
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
    "Snake played on the surface of a Klein bottle: leaving one edge "
    "brings you back mirrored on the other.", "",
    "Directions: steer. Select: start / pause (A). Sel+Down: B.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Nielk",
    "",
    "Greyson Potter (gkpotter), 2020.",
    "",
    "https://github.com/gkpotter/Nielk @ d577698",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "nielk", "Nielk", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=steer  Sel=A  Sel+Dn=B|Hold Sel+Up=menu"
};
