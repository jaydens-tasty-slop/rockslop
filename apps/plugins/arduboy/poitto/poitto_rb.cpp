/* Rockbox game description for POITTO. Select = A (jump). */
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
    "An action puzzle game: reach the goal of each stage, using "
    "springs, arrows and switches.", "",
    "Left/Right: move. Select: jump (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "POITTO",
    "",
    "@ina_ani (inajob), 2016.",
    "",
    "https://github.com/ArduboyCollection/poitto @ 8e0f8b9",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "poitto", "POITTO", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=jump|Hold Sel+Up=menu"
};
