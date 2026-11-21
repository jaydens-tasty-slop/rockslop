/* Rockbox game description for Omega Chase. Select = B (full auto), Select+Down = A (single shot). */
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
    "An Omega Race style arcade shooter: bounce around the arena and "
    "shoot the drones, command ships and mines.", "",
    "Left/Right: rotate. Up: thrust. Down: reverse thrust.", "",
    "Select: full-auto fire (B).", "",
    "Select + Down: single shot (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Omega Chase",
    "",
    "Karl P. Williams (2023).",
    "",
    "https://github.com/Karl-Williams/OmegaChase @ 64bc7d3",
    "",
    "License: MIT. Music and effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "omegachase", "Omega Chase", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=auto  Sel+Down=single|Hold Sel+Up=menu"
};
