/* Rockbox game description for Blade Runner. Select = A (confirm / menu), Select+Down = B (flip / extend the blade). */
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
    "Extend the blade bridge to the next platform; hit its centre for "
    "coins; buy heroes in the store.", "",
    "Left/Right: choose in menus.", "",
    "Select: confirm (A).", "",
    "Select + Down: hold to extend the blade, tap to flip (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Blade Runner",
    "",
    "Popov Ilia (NoobGeek), 2022.",
    "",
    "https://github.com/NoobGeek-Ilia/BladeRunner @ cf6db4e",
    "",
    "License: Apache 2.0. Effects through ArduboyTones (BeepPin1 routed "
    "to them).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "bladerunner", "Blade Runner", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=confirm  Sel+Down=blade|Hold Sel+Up=menu"
};
