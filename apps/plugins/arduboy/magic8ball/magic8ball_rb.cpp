/* Rockbox game description for Magic 8 Ball. Select = A (ask the ball). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Ask a yes/no question and let the Magic 8 Ball answer.", "",
    "Select: ask the ball (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Magic 8 Ball",
    "",
    "HugoQuadrado (2019).",
    "",
    "https://github.com/HugoQuadrado82/Arduboy8ball @ 0ac1459",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "magic8ball", "Magic 8 Ball", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=ask the ball|Hold Sel+Up=menu"
};
