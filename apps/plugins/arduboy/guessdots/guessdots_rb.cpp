/* Rockbox game description for Guess Dots. Select = A (toggle/confirm); Select+Down = B. */
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
    "A Minesweeper-like puzzle: reveal the dot pattern that satisfies "
    "the numbers.", "",
    "Directions: move. Select: toggle dot / confirm (A). Sel+Down: B.", "",
    "On the title screen Up+Down resets the score.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Guess Dots",
    "",
    "Sergei Petrov (SuperSolik), 2023. Idea from the iOS game Dot by "
    "OuzzGame.",
    "",
    "https://github.com/SuperSolik/arduboy-games @ 523b23a (guess_dots)",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "guessdots", "Guess Dots", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=toggle|Sel+Dn=B|Hold Sel+Up=menu"
};
