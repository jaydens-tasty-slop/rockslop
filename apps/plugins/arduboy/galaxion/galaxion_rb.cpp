/* Rockbox game description for Galaxion. Fire is A or B, so the default mapping works: SELECT = A, SELECT + DOWN = B. */
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
    "A Galaxian style shooter: destroy the waves of aliens diving at "
    "your ship.", "",
    "Left/Right: move the ship.", "",
    "Select: fire (A or B); starts the game.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Galaxion",
    "",
    "tako2 (2017).",
    "",
    "https://github.com/tako2/galaxion",
    "",
    "License: MIT. The start jingle and effects are Playtune scores "
    "played through the layer.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "galaxion", "Galaxion", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Left/Right=move  Sel=fire|Hold Sel+Up=menu"
};
