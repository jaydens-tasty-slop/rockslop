/* Rockbox game description for Poop Panic!. Only A is used: SELECT = A. */
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
    "Clean up after the animals before the poop piles up.", "",
    "Directions: move.", "",
    "Select: start the game / clean up (A).", "",
    "Up/Down on the title: choose the level.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Poop Panic!",
    "",
    "inajob (2018).",
    "",
    "https://github.com/ArduboyCollection/poop-panic @ c0c44a8",
    "",
    "License: MIT. Music and effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "poopanic", "Poop Panic!", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A|Hold Sel+Up=menu"
};
