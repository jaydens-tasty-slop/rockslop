/* Rockbox game description for Night Raid. A fires the left battery, B the right: tap SELECT = A, hold SELECT = B. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LONG, AB_B_B},
    {AB_P_TAP,                AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A Missile Command style game: defend your cities from the incoming "
    "missiles by exploding them with your own.", "",
    "Directions: move the crosshair.", "",
    "Tap Select: fire a missile from the left battery (A).", "",
    "Hold Select (0.3 s): fire from the right battery (B).", "",
    "Hold Select + Up for 1 s: pause menu (the 0.3 s hold is the B button plus Up).",
    NULL
};

static const char *const credits[] = {
    "Night Raid",
    "",
    "Evan Barger (2016-2018).",
    "",
    "https://github.com/epbarger/night-raid",
    "",
    "License: MIT. The game has no sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "nightraid", "Night Raid", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 1000, 300, 0, 0, NULL, 0,
    "Tap=left  Hold=right|Hold Sel+Up=menu"
};
