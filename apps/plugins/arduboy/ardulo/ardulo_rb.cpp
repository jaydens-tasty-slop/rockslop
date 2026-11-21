/* Rockbox game description for ArduLO. Select = B (toggle light, confirm), Select+Down = A (pause). */
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
    "A Lights Out puzzle: toggle a light and its neighbours until all "
    "are off.", "",
    "Directions: move the cursor.", "",
    "Select: toggle the light / confirm (B).", "",
    "Select + Down: pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "ArduLO",
    "",
    "Jon Thysell (2020-2021).",
    "",
    "https://github.com/jonthysell/ArduLO @ 34d3b87",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardulo", "ArduLO", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=toggle  Sel+Down=pause|Hold Sel+Up=menu"
};
