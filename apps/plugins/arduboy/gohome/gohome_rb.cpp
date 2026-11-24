/* Rockbox game description for Go Home. Select = A (undo; new game on the title), Select+Down = B (restart level; continue on the title). */
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
    "Slide your three friends home (the white squares); they all move "
    "the same way.", "",
    "Directions: slide.", "",
    "Select: undo (A); new game on the title.", "",
    "Select + Down: restart the level (B); continue on the title.", "",
    "60 levels.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Go Home",
    "",
    "Frank van de Ven (2026); idea after Hakosan by ABAgames.",
    "",
    "https://github.com/FrankVanDeVen/GoHome @ 4fda4c8",
    "",
    "License: MIT. No sound upstream. Progress is saved in the per-game "
    "save file.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "gohome", "Go Home", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=undo  Sel+Down=restart|Hold Sel+Up=menu"
};
