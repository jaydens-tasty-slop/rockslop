/* Rockbox game description for Row Farm. Select = A (add/cycle symbol), Select+Down = B (pause menu); Left = accelerate. */
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
    "A puzzle game inspired by Columns: line up the farm symbols in "
    "rows to clear them before the field fills.", "",
    "Up / Down: move the cursor.", "",
    "Left: accelerate.", "",
    "Select: cycle the symbol position (A).", "",
    "Select + Down: pause menu (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Row Farm",
    "",
    "Glitsch3n (2024).",
    "",
    "https://github.com/Glitsch3n/arduboy-game-collection @ b1ffab7 "
    "(RowFarm)",
    "",
    "License: MIT. Beeps through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "rowfarm", "Row Farm", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "U/D=move  Sel=cycle|Hold Sel+Up=menu"
};
