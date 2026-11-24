/* Rockbox game description for ArduBloxx. Select = B (drop a block / confirm), Select+Down = A. */
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
    "Build a city in 13 days: stack blocks into buildings and keep the "
    "tower steady.", "",
    "Directions: choose in the town and menus.", "",
    "Select: drop a block / confirm (B).", "",
    "Select + Down: A.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "ArduBloxx",
    "",
    "Popov Ilia (NoobGeek), 2023; a remake of City Bloxx.",
    "",
    "https://github.com/NoobGeek-Ilia/ARDUBLOXX @ eec69d9",
    "",
    "License: Apache 2.0. Effects through ArduboyTones (BeepPin1 routed "
    "to them).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardubloxx", "ArduBloxx", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=drop  Arrows=choose|Hold Sel+Up=menu"
};
