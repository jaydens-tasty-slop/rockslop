/* Rockbox game description for TeenyTank. B fires, A (held) gives fine aim/power: tap SELECT = B, hold SELECT = A. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LONG, AB_B_A},
    {AB_P_TAP,                AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A two-player artillery duel on randomly generated hills: take "
    "turns to aim and fire.", "",
    "Left/Right: aim. Up/Down: set the power.", "",
    "Tap Select: fire (B).", "",
    "Hold Select (0.3 s): fine aim and power (A) together with the "
    "directions.", "",
    "Hold Select + Up for 1 s: pause menu (the 0.3 s hold is the A button plus Up).",
    NULL
};

static const char *const credits[] = {
    "TeenyTank",
    "",
    "Jez Swanson (2020).",
    "",
    "https://github.com/Jezzamonn/TeenyTank",
    "",
    "License: MIT. Two players take turns; startup tune and effects via "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "teenytank", "TeenyTank", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 1000, 300, 0, 0, NULL, 0,
    "Tap=fire  Hold=fine aim|Hold Sel+Up=menu"
};
