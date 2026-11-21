/* Rockbox game description for Military Aviator. Select = A (fire), Select+Down = B (back / continue). */
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
    "A tiny jet shooter: shoot enemy planes.", "",
    "Directions: fly.", "",
    "Select: fire (A).", "",
    "Select + Down: back to the menu (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Military Aviator",
    "",
    "curlyc (2017).",
    "",
    "https://github.com/curlyc/militaryaviator @ 303b417",
    "",
    "License: MIT. No sound upstream. The author calls it rough but "
    "playable.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "militaryaviator", "Military Aviator", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=back|Hold Sel+Up=menu"
};
