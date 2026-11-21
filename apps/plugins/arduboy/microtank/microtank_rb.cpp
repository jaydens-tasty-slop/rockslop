/* Rockbox game description for MicroTank. Select = A (fire), Select+Down = B, Select+Left = A+B (special attack). */
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
    "Drive your tank, shoot enemies, collect coins and upgrade in the "
    "tank shop.", "",
    "Directions: drive.", "",
    "Select: fire (A).", "",
    "Select + Down: B.", "",
    "Select + Left: special attack (A+B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "MicroTank",
    "",
    "Thomas Hartmann (2019).",
    "",
    "https://github.com/hartmann1301/MicroTank @ 1baad6c",
    "",
    "License: Apache 2.0. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "microtank", "MicroTank", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Left=special|Hold Sel+Up=menu"
};
