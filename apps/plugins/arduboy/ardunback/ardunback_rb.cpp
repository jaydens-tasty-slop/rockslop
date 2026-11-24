/* Rockbox game description for Ardu N-Back. Select = B, Select+Down = A (both mean Other). */
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
    "A dual 2-back memory task: a letter appears at a grid position; "
    "compare it with the one two steps back.", "",
    "Up: both are the same.", "",
    "Left: same position.", "",
    "Right: same letter.", "",
    "Down: all different.", "",
    "Select: none of these / start.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ardu N-Back",
    "",
    "Ruben Berenguel Montoro (2023).",
    "",
    "https://github.com/rberenguel/ardu-n-back @ 029f518",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardunback", "Ardu N-Back", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=answer  Sel=other|Hold Sel+Up=menu"
};
