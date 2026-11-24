/* Rockbox game description for Outpost Defense. B places a trooper, A pauses: SELECT = B, SELECT + DOWN = A. */
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
    "Send troopers across the field to take the enemy outpost before "
    "time runs out.", "",
    "Directions: move the pointer.", "",
    "Select: place a trooper (B); confirms in menus.", "",
    "Select + Down: pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Outpost Defense",
    "",
    "TEAM a.r.g.: Trodoss (2017).",
    "",
    "https://github.com/Team-ARG-Museum/ID-49-Outpost-Defense",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "outpostdefense", "Outpost Defense", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=trooper  Sel+Down=pause|Hold Sel+Up=menu"
};
