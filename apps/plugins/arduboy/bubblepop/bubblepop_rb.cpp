/* Rockbox game description for Bubble PoP. B shoots, A pauses: SELECT = B, SELECT + DOWN = A. */
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
    "A bubble shooter: aim and fire bubbles to pop groups of three or "
    "more of the same colour.", "",
    "Directions: move / aim.", "",
    "Select: shoot (B); confirms in menus.", "",
    "Select + Down: pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Bubble PoP",
    "",
    "TEAM a.r.g.: Gaveno, JO3RI (2018).",
    "",
    "https://github.com/Team-ARG-Museum/ID-14-Bubble-PoP",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "bubblepop", "Bubble PoP", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=shoot  Sel+Down=pause|Hold Sel+Up=menu"
};
