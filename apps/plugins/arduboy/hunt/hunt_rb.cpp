/* Rockbox game description for HUNT!. Select = B (shoot / confirm), Select+Down = A (start / back to the menu). */
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
    "Survive in a top-down arena: dodge the enemies and shoot them.", "",
    "Directions: move.", "",
    "Select: shoot / confirm (B).", "",
    "Select + Down: start / back to the menu (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "HUNT!",
    "",
    "Volodymyr Kadzhaia (ladroid) (2021).",
    "",
    "https://github.com/ladroid/HUNT @ 03b27f8",
    "",
    "License: BSD 3-Clause. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "hunt", "HUNT!", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=shoot  Sel+Down=back|Hold Sel+Up=menu"
};
