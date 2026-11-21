/* Rockbox game description for Peg Solitaire (Frank Van De Ven). SELECT = B , SELECT + DOWN = A. */
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
    "Clear the pegs until only the middle one remains.", "",
    "Directions: move cursor.  Select + Down (A): pick a peg, then jump to an empty hole.", "",
    "Select (B): undo the last jump.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Peg Solitaire", "",
    "Frank Van De Ven, github.com/FrankVanDeVen/PegSolitaire", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "pegsolitaire", "Peg Solitaire", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel+Dn=pick Sel=undo|Sel+Up=menu"
};
