/* Rockbox game description for Asteroids (CDR_Xavier). Select = B (shoot), Select+Down = A (pause). */
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
    "Shoot the asteroids, dodge the rocks, and use hyperspace in an "
    "emergency.", "",
    "Left/Right: turn. Up: thrust. Down: hyperspace.", "",
    "Select: shoot (B).", "",
    "Select + Down: pause (A); while paused Select resumes and Select + "
    "Down exits.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Asteroids (CDR_Xavier)",
    "",
    "CDR_Xavier (2018).",
    "",
    "https://github.com/CDRXavier/ASTEROID @ 68dedff",
    "",
    "License: GPL 2 or later. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "asteroidsx", "Asteroids (CDR_Xavier)", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=shoot  Sel+Down=pause|Hold Sel+Up=menu"
};
