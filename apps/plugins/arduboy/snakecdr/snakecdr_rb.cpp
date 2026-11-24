/* Rockbox game description for Snake. Select = B (start), Select+Down = A (high scores). */
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
    "The classic snake game.", "",
    "Directions: steer.", "",
    "Select: start (B).", "",
    "Select + Down: high scores (A).", "",
    "Up/Down on the title: sound on/off.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Snake",
    "",
    "CDR_Xavier (2018).",
    "",
    "https://github.com/CDRXavier/SNAKE @ 501d6a4",
    "",
    "License: GPL 2 or later. Effects through the v1 tunes shim over "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "snakecdr", "Snake (CDR_Xavier)", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=start  Sel+Down=scores|Hold Sel+Up=menu"
};
