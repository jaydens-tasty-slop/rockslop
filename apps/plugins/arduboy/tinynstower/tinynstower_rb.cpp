/* Rockbox game description for Tiny NS-Tower. Select = A (jump); Left also jumps. */
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
    "Climb the tower to the 30th floor.", "",
    "Select or Left: jump (A).", "",
    "Up: reset.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiny NS-Tower",
    "",
    "akkera102 (2017).",
    "",
    "https://github.com/akkera102/04_tiny_ns_tower @ 0ee5827",
    "",
    "License: GPL 2 (the repository carries the GPL text). Effects "
    "through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tinynstower", "Tiny NS-Tower", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel/Left=jump  Up=reset|Hold Sel+Up=menu"
};
