/* Rockbox game description for Slide-XL. Select = A (choose / action), Select+Down = B (action). */
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
    "Sliding puzzle: 8 digits (3x3), 15 digits (4x4), or a picture "
    "(4x4).", "",
    "Directions: slide a tile.", "",
    "Select: choose / action (A).", "",
    "Select + Down: action (B); the game explains the buttons.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Slide-XL",
    "",
    "Frank van de Ven (2026).",
    "",
    "https://github.com/FrankVanDeVen/Slide-XL @ 0b0b160",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "slidexl", "Slide-XL", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
