/* Rockbox game description for Tiny Dash. Select = A (continue / retry), Select+Down = B (info, new game). */
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
    "A Boulder Dash clone: dig through the cave and collect all "
    "diamonds; 18 levels.", "",
    "Directions: walk.", "",
    "Select: continue / retry the level (A).", "",
    "Select + Down: info, new game from level 1 (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiny Dash",
    "",
    "Frank van de Ven (2023).",
    "",
    "https://github.com/FrankVanDeVen/Tiny-Dash @ 30131ce",
    "",
    "License: MIT. No sound upstream. The level is kept in the per-game "
    "save file.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tinydash", "Tiny Dash", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=retry  Sel+Down=info|Hold Sel+Up=menu"
};
