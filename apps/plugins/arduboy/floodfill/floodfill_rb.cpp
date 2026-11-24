/* Rockbox game description for Flood Fill. Select = A (flood), Select+Down = B (back to title). */
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
    "Flood the whole board with one pattern within 20 moves; 999 "
    "levels.", "",
    "Directions: choose a pattern.", "",
    "Select: flood with the chosen pattern (A); starts the game.", "",
    "Select + Down: back to the title (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Flood Fill",
    "",
    "Gary Franz (2018).",
    "",
    "https://github.com/garyfranz/arduboy_floodfill @ 471503d",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "floodfill", "Flood Fill", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=flood  Sel+Down=title|Hold Sel+Up=menu"
};
