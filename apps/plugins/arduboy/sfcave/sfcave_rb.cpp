/* Rockbox game description for SFCave. Only A is used: Select = A. */
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
    "Fly through a narrowing cave: hold the button to climb, release to "
    "fall.", "",
    "Select (A): climb; starts the game.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "SFCave",
    "",
    "Stephanie Maks (Arduino), Rodney Norton (Arduboy); inspired by "
    "Sunflat SFCave for Palm.",
    "",
    "https://github.com/ArduboyCollection/SFCave @ 3cbb554",
    "",
    "License: GPL 3. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "sfcave", "SFCave", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=climb (hold)|Hold Sel+Up=menu"
};
