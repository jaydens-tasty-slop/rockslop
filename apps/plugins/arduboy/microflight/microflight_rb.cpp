/* Rockbox game description for MicroFlight. Select = B (cycle draw mode); directions pitch and roll. */
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
    "A tiny 3D flight simulator demo: fly over textured terrain.", "",
    "Up / Down: pitch.", "",
    "Left / Right: roll.", "",
    "Select: cycle the draw method (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "MicroFlight",
    "",
    "James Howard (jhhoward), 2019.",
    "",
    "https://github.com/jhhoward/MicroFlight @ 7061957",
    "",
    "License: MIT. Sound through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "microflight", "MicroFlight", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=fly  Sel=drawmode|Hold Sel+Up=menu"
};
