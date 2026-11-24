/* Rockbox game description for ArduSkiing. Select = A (save score), Select+Down = B (start / new game). */
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
    "Ski between as many flags as you can in 2 minutes; passing a flag "
    "scores, missing or hitting one costs a point.", "",
    "Left/Right: steer.", "",
    "Select + Down: start / new game (B).", "",
    "Select: save the score at the end (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "ArduSkiing",
    "",
    "Frank van de Ven (2026).",
    "",
    "https://github.com/FrankVanDeVen/ArduSkiing @ 1ee2239",
    "",
    "License: MIT. No sound upstream. The best score is kept in the "
    "per-game save file.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arduskiing", "ArduSkiing", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Left/Right=steer  Sel+Dn=go|Hold Sel+Up=menu"
};
