/* Rockbox game description for Ravine Despoiler. Only A is used: Select = A (drop bomb). */
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
    "After Canyon Bomber: fly over the ravine and bomb the boulders.", "",
    "Left/Right: slow down or speed up.", "",
    "Select: drop a bomb (A); starts the game.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ravine Despoiler",
    "",
    "Ben Combee (2020-2021); physics adapted from Pharap's "
    "PhysixArduboy.",
    "",
    "https://github.com/unwiredben/arduboy-ravine-despoiler @ c48915c",
    "",
    "License: Apache 2.0. Music and effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ravine", "Ravine Despoiler", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=bomb|Hold Sel+Up=menu"
};
