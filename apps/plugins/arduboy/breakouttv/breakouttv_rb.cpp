/* Rockbox game description for Breakout (Vlad). Select = A (launch the ball / start), Select+Down = B (pause). */
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
    "A basic Breakout clone.", "",
    "Left/Right: move the paddle.", "",
    "Select: start / launch the ball (A).", "",
    "Select + Down: pause (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Breakout (Vlad)",
    "",
    "Vlad Tomoiaga (2020).",
    "",
    "https://github.com/tvlad1234/breakout @ a5c1e49",
    "",
    "License: GPL 3. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "breakouttv", "Breakout (Vlad)", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=launch  Sel+Down=pause|Hold Sel+Up=menu"
};
