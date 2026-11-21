/* Rockbox game description for Little Beluga. Select = A (start; any button starts). */
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
    "Swim the little beluga through the gaps between the obstacles.", "",
    "Up/Down: swim.", "",
    "Select: start (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Little Beluga",
    "",
    "Lincoln Hach Yellick (2020).",
    "",
    "https://github.com/lyellick/LittleBeluga @ 5bbaa08",
    "",
    "License: MIT. Effects and music through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "littlebeluga", "Little Beluga", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up/Down=swim  Sel=start|Hold Sel+Up=menu"
};
