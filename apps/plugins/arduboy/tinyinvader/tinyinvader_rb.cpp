/* Rockbox game description for Tiny Digital Invader. Select = B (fire), Select+Down = A (restart after game over). */
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
    "A clone of the Casio MG-880 calculator game, drawn on eight "
    "7-segment digits; 18 stages.", "",
    "Left: select / move.", "",
    "Select: fire (B).", "",
    "Select + Down: restart after game over (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiny Digital Invader",
    "",
    "akkera102 (2020).",
    "",
    "https://github.com/akkera102/10_tiny_invader @ 1abefdb",
    "",
    "License: GPL 2 (the repository carries the GPL text). No sound "
    "upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tinyinvader", "Tiny Digital Invader", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Left=move|Hold Sel+Up=menu"
};
