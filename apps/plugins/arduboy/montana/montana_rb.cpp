/* Rockbox game description for Montana. Select = A (move card / hint), Select+Down = B (reshuffle). */
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
    "Montana (Gaps) Solitaire: put the 52 cards in four ordered rows, 2 "
    "to king.", "",
    "Directions: choose a card.", "",
    "Select: move the card to its gap (A); hint if enabled.", "",
    "Select + Down: reshuffle (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Montana",
    "",
    "Frank van de Ven (2026).",
    "",
    "https://github.com/FrankVanDeVen/Montana @ 35810a1",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "montana", "Montana", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=move  Sel+Down=shuffle|Hold Sel+Up=menu"
};
