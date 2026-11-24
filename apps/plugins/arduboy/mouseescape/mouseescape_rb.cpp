/* Rockbox game description for Mouse Escape. Select = A (start / pause), Select+Down = B unused. */
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
    "You are a field mouse: collect food, dodge the snakes and get back "
    "home.", "",
    "Directions: move the mouse.", "",
    "Select: start / pause (A).", "",
    "Ten pieces of food give an extra life.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Mouse Escape",
    "",
    "Zachariah Falgout (RetrobitCoder), 2018.",
    "",
    "Removed: the title banner and lose screen by LENZ (no licence); "
    "text replaces them.",
    "",
    "https://github.com/RetrobitCoder/MouseEscape @ 058e19c",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "mouseescape", "Mouse Escape", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=start/pause|Hold Sel+Up=menu"
};
