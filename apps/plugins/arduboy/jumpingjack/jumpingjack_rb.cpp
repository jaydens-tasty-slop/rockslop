/* Rockbox game description for Jumping Jack. Select = A (jump / start), Select+Down = B (shoot). */
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
    "Run left and right and jump through the gaps in the rising floors.", "",
    "Left/Right: run.", "",
    "Select: jump / confirm (A).", "",
    "Select + Down: shoot (B), extended version.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Jumping Jack",
    "",
    "marggines (2023).",
    "",
    "https://github.com/marggines/JumpingJack @ d9ba382",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "jumpingjack", "Jumping Jack", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=shoot|Hold Sel+Up=menu"
};
