/* Rockbox game description for Pinball. B = right flipper, Left = left flipper, A = plunger: SELECT = B, SELECT + DOWN = A. */
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
    "A pinball table with scrolling view: keep the ball in play and "
    "score.", "",
    "Left: left flipper.", "",
    "Select: right flipper (B); confirms in menus.", "",
    "Select + Down: launch the ball with the plunger (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pinball",
    "",
    "TEAM a.r.g.: Grapple Bug, Gaveno, JO3RI, Trodoss (2017).",
    "",
    "https://github.com/Team-ARG-Museum/ID-44-Pinball",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "pinball", "Pinball", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=flip R  Left=flip L|Hold Sel+Up=menu"
};
