/* Rockbox game description for Randocity. Select = B (accelerate), Select+Down = A (brake), Select+Right = Down+A (pause). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_RIGHT, AB_B_A | AB_B_DOWN},
    {AB_P_SELECT | AB_P_DOWN, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Ride a motorbike through a huge random city: free play, race to "
    "the edge, or crazy courier.", "",
    "Left/Right: turn.", "",
    "Select: accelerate (B, press repeatedly).", "",
    "Select + Down: brake (A).", "",
    "Select + Right: pause (Down+A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Randocity",
    "",
    "Paul Wasson (pmwasson), 2019.",
    "",
    "https://github.com/pmwasson/Randocity @ 42e9b7e",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "randocity", "Randocity", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=gas  Sel+Down=brake|Sel+Right=pause|Hold Sel+Up=menu"
};
