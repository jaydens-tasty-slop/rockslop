/* Rockbox game description for Ardutan. Select = B (shoot on release), Select+Down = A (fine aim). */
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
    "A block-breaker: aim left and right, then shoot balls at the "
    "descending blocks before they reach the bottom.", "",
    "Left/Right: aim.", "",
    "Select: shoot, on release (B).", "",
    "Select + Down: hold for fine aim (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ardutan",
    "",
    "yamasakihiromi (2017).",
    "",
    "https://github.com/yamasakihiromi/ardutan @ e475f36",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardutan", "Ardutan", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=aim  Sel=fire|Hold Sel+Up=menu"
};
