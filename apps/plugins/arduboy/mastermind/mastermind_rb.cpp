/* Rockbox game description for Mastermind (Frank Van De Ven). SELECT = B , SELECT + DOWN = A. */
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
    "Crack the secret code of four different numbers (1-6) in nine tries.", "",
    "Up / Down: pick a pawn.  Left / Right: change its number.", "",
    "Select + Down: submit the guess.  Select: new game.", "",
    "Black = right number, right place.  White = right number, wrong place.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Mastermind", "",
    "Frank Van De Ven, github.com/FrankVanDeVen/Mastermind", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "mastermind", "Mastermind", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel+Down=guess  Sel=new|Sel+Up=menu"
};
