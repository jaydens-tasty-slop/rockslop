/* Rockbox game description for Flappy Halloween. Select = A (start); Up/Down fly. */
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
    "A bat flies through the night: eat the bugs and avoid the owl. "
    "Concept and art by the author's 10-year-old.", "",
    "Up / Down: fly.", "",
    "Select: start (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Flappy Halloween",
    "",
    "Andrew Low, 2015 (concept and art by his 10-year-old).",
    "",
    "https://github.com/andrewlow/flappyhalloween @ 94f926c",
    "",
    "License: Eclipse Public License 1.0. Effects through the v1 tunes "
    "shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "flappyhalloween", "Flappy Halloween", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up/Down=fly  Sel=start|Hold Sel+Up=menu"
};
