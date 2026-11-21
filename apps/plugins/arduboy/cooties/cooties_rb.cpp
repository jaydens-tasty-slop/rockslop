/* Rockbox game description for Cooties Attack. Select = A (shoot/start); directions move. */
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
    "The cooties are attacking, and your shooting controls are bugged. "
    "Beat them all anyway.", "",
    "Directions: move.", "",
    "Select: shoot / start (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Cooties Attack",
    "",
    "The Programming Raccoon (2021); Font3x5 by Filmote.",
    "",
    "https://github.com/Pharap/Cooties-Attack @ 2978d2f",
    "",
    "License: MIT; the font is BSD-3-Clause. Effects through "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "cooties", "Cooties Attack", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=shoot|Hold Sel+Up=menu"
};
