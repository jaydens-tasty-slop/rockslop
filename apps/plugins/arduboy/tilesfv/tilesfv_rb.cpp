/* Rockbox game description for Tiles. Select = A (pick / place a tile), Select+Down = B (new tiles). */
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
    "Place tiles so they fill a whole row or column; full lines vanish.", "",
    "Directions: move the tile.", "",
    "Select: pick up / place the tile (A).", "",
    "Select + Down: three new tiles (B); the fourth time ends the game.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiles",
    "",
    "Frank van de Ven (2026).",
    "",
    "https://github.com/FrankVanDeVen/Tiles @ 7dfa3c4",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tilesfv", "Tiles", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=place  Sel+Down=more|Hold Sel+Up=menu"
};
