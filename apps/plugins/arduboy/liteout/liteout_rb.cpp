/* Rockbox game description for Lite Out. Select = A (flip, start), Select+Down = B (give up). */
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
    "Turn off all the lights: flipping a square also flips its four "
    "neighbours.", "",
    "Directions: move the cursor.", "",
    "Select: flip (A) and start.", "",
    "Select + Down: give up (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Lite Out",
    "",
    "Lite Out for the Arduboy (see the forum post upstream).",
    "",
    "https://github.com/ArduboyCollection/ArduboyLiteOut @ 75a4e63",
    "",
    "License: Unlicense (public domain). The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "liteout", "Lite Out", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=flip  Sel+Down=give up|Hold Sel+Up=menu"
};
