/* Rockbox game description for Reverse Mermaid Hockey. B launches the ball, A pauses: SELECT = B, SELECT + DOWN = A. */
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
    "Air-hockey with a mermaid: score against the computer opponent "
    "before it scores on you.", "",
    "Directions: move your mermaid.", "",
    "Select: launch the ball (B); confirms in menus.", "",
    "Select + Down: pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Reverse Mermaid Hockey",
    "",
    "TEAM a.r.g.: Dragula96, art JO3RI and Unclesporky (2015).",
    "",
    "https://github.com/Team-ARG-Museum/ID-31-RMH",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "rmh", "Reverse Mermaid Hockey", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=launch  Sel+Down=pause|Hold Sel+Up=menu"
};
