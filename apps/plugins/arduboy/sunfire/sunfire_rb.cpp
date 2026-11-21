/* Rockbox game description for Sunfire. B fires guns (hold), A fires plasma: SELECT = B, SELECT + DOWN = A. */
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
    "A first-person space flight shooter: pilot your ship through the "
    "waves of enemies.", "",
    "Directions: steer and roll.", "",
    "Select: fire the guns (B, hold). Confirms in menus.", "",
    "Select + Down: fire plasma (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Sunfire",
    "",
    "TEAM a.r.g.: Trodoss (2017).",
    "",
    "https://github.com/Team-ARG-Museum/ID-50-Sunfire",
    "",
    "Code, art and music: MIT licence, (c) TEAM a.r.g. "
    "Music player: ATMlib, BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "sunfire", "Sunfire", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=guns  Sel+Down=plasma|Hold Sel+Up=menu"
};
