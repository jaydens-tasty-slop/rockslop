/* Rockbox game description for Particles' Segregation. Select = A; Select+Down = B. */
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
    "Separate protons and electrons by storing them in their boxes "
    "before they explode. Five levels of rising difficulty.", "",
    "Directions: move. Select: A. Sel+Down: B.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Particles' Segregation",
    "",
    "Entry for a 64 hour game jam by MariEss (the licence text names no "
    "holder), 2019.",
    "",
    "https://github.com/MariEss/GJM_Particles-_Segregation @ 8c9e0b8",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "particles", "Particles' Segregation", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=A  Sel+Dn=B|Hold Sel+Up=menu"
};
