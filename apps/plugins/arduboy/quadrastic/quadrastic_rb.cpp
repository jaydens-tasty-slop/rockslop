/* Rockbox game description for Quadrastic. Select = A (start), Select+Down = B (sound options / pause). */
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
    "Collect the targets with your square before time runs out.", "",
    "Directions: move.", "",
    "Select: start the game (A).", "",
    "Select + Down: sound options on the title, pause in the game (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Quadrastic",
    "",
    "David Martinez (code), DarKaoz (graphics), 2015-2017.",
    "",
    "https://github.com/ArduboyCollection/Quadrastic @ 9d36cc5",
    "",
    "License: LGPL 2.1 or later (header), GPL 3 text in the repository. "
    "Music through Playtune.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "quadrastic", "Quadrastic", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=start  Sel+Down=pause|Hold Sel+Up=menu"
};
