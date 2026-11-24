/* Rockbox game description for 1010 for Arduboy. Select = A (choose/place), Select+Down = B (back); directions move. */
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
    "Block puzzle after 1010!: place the three offered pieces on the "
    "10x10 board; full rows and columns clear.", "",
    "Left/Right: pick a piece. Directions: move it.", "",
    "Select: choose / place (A).", "",
    "Select + Down: back to the piece choice (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "1010 for Arduboy",
    "",
    "joshua stein (jcs), 2018.",
    "",
    "https://github.com/jcs/arduboy-1010 @ 2519bab",
    "",
    "License: ISC (permissive). The game has no sound; progress is "
    "saved.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arduboy1010", "1010 for Arduboy", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=place|Hold Sel+Up=menu"
};
