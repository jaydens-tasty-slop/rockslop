/* Rockbox game description for Tiny Asteroids. Select = A (shoot), Select+Down = B (thrust). */
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
    "Asteroids (1979) clone in zero gravity; five stages, UFOs "
    "included.", "",
    "Left/Right: rotate.", "",
    "Up: warp to a random spot.", "",
    "Select: shoot (A).", "",
    "Select + Down: thrust (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiny Asteroids",
    "",
    "akkera102 (2017).",
    "",
    "https://github.com/akkera102/06_tiny_asteroids @ b8baea4",
    "",
    "License: GPL 2 (the repository carries the GPL text). Effects "
    "through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tinyasteroids", "Tiny Asteroids", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=shoot  Sel+Down=thrust|Hold Sel+Up=menu"
};
