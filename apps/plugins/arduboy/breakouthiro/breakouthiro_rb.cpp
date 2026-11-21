/* Rockbox game description for Breakout (Hiroshi). Select = A (fast paddle), Select+Down = B (add a ball). */
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
    "Breakout with up to eight balls in play.", "",
    "Left/Right: move the paddle.", "",
    "Select: hold for a faster paddle (A).", "",
    "Select + Down: add a ball (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Breakout (Hiroshi)",
    "",
    "Hiroshi Tamura (2018).",
    "",
    "https://github.com/HiroshiTamura/arduboy_breakout_game @ 5c1ebe8",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "breakouthiro", "Breakout (Hiroshi)", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fast  Sel+Down=new ball|Hold Sel+Up=menu"
};
