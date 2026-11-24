/* Rockbox game description for JumpBoy. Select = B (start), directions move; any button starts. */
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
    "Collect twenty coins while the floorboards disappear one by one "
    "with every jump.", "",
    "Left/Right: move. Up: jump.", "",
    "Select: start (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "JumpBoy",
    "",
    "takaiwa / t-iwasaki (2016, 2020).",
    "",
    "https://github.com/t-iwasaki/arduboy-JumpBoy @ 1b2dcc9",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "jumpboy", "JumpBoy", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=start|Hold Sel+Up=menu"
};
