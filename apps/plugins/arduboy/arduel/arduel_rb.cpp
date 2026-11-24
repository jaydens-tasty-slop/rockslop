/* Rockbox game description for Arduel. Directions only; Select unused. */
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
    "A small platform game: climb the platforms and avoid the enemies.", "",
    "Directions: move.", "",
    "The game uses no other buttons.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Arduel",
    "",
    "curlyc (2017).",
    "",
    "https://github.com/curlyc/arduel @ 1f770d4",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arduel", "Arduel", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Directions=move|Hold Sel+Up=menu"
};
