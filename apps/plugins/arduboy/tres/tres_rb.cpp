/* Rockbox game description for Tres. Select = B (everything), A is only used with B to leave a screen. */
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
    "A Threes! style sliding number puzzle: merge 1s and 2s into 3s and "
    "bigger.", "",
    "Directions: slide the tiles.", "",
    "Select: confirm / pause (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tres",
    "",
    "Casey Gold (cajogold), 2017.",
    "",
    "https://github.com/ArduboyCollection/tres @ 8befbc9",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tres", "Tres", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=OK/pause|Hold Sel+Up=menu"
};
