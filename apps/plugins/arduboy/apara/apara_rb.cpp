/* Rockbox game description for APara. Left/Right (or Select = right/B, Select+Down = left/A) move the boat. */
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
    "A parachute game: move the boat to catch the paratroopers before "
    "they land in the sea.", "",
    "Left/Right: move the boat.", "",
    "Select: right (B); Select + Down: left (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "APara",
    "",
    "Leonhard Vogt (2016).",
    "",
    "https://github.com/ArduboyCollection/APara @ b1e1042",
    "",
    "License: GPL 3. Effects through the v1 tunes shim over "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "apara", "APara", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=move  Sel=right|Hold Sel+Up=menu"
};
