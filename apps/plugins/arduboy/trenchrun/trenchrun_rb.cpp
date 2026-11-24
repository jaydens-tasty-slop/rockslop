/* Rockbox game description for Trench Run. Select = B (fire), Select+Down = A. */
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
    "Fly the trench and shoot the TIE fighters before they hit you.", "",
    "Directions: move the crosshair.", "",
    "Select: fire (B); advances the screens.", "",
    "Select + Down: pause the game (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Trench Run",
    "",
    "Lucas Salton Cardinali (2019).",
    "",
    "https://github.com/lscardinali/TrenchRun-Arduboy @ 2012f98",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "trenchrun", "Trench Run", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=pause|Hold Sel+Up=menu"
};
