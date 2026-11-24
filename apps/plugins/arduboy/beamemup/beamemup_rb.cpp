/* Rockbox game description for Beam 'Em Up. Only A is used: Select = A (beam). */
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
    "Fly the saucer and beam up the cows before time runs out.", "",
    "Directions: fly.", "",
    "Select: beam (A, hold).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Beam 'Em Up",
    "",
    "Ben Combee (2020); physics adapted from Pharap's PhysixArduboy.",
    "",
    "https://github.com/unwiredben/arduboy-beamemup @ 95a7fb6",
    "",
    "License: Apache 2.0. Music and effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "beamemup", "Beam 'Em Up", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=beam (hold)|Hold Sel+Up=menu"
};
