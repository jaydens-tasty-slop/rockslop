/* Rockbox game description for Creapawn. SELECT = B, SELECT + DOWN = A. */
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
    "Collect creatures (pawns), build a team and win chess-like battles.", "",
    "Directions: move and choose.", "",
    "Select: B (back / cancel).", "",
    "Select + Down: A (confirm / act).", "",
    "Progress is saved by the game.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Creapawn", "",
    "Revlis, github.com/Revlis64/Creapawn.", "",
    "ArdBitmap by Ignacio Vina (Apache-2.0).", "",
    "License: Apache-2.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "creapawn", "Creapawn", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=B  Sel+Down=A|Sel+Up=menu"
};
