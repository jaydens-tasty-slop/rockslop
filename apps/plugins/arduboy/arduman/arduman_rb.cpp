/* Rockbox game description for Ardu-man. A selects, B is back in the name entry: SELECT = A, SELECT + DOWN = B. */
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
    "A Pac-Man style maze game: eat all the dots, grab the fruit and "
    "escape the ghosts.", "",
    "Directions: move; in the menu Up/Down choose.", "",
    "Select: choose a menu entry (A), enter a name letter.", "",
    "Select + Down: back / delete a letter in the name entry (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ardu-man",
    "",
    "Seth A. Robinson (2016).",
    "",
    "https://github.com/SethRobinson/arduman_arduboylib11",
    "",
    "License: MIT. Music and effects are played through the layer's "
    "Playtune and tone engines.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arduman", "Ardu-man", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=select  Sel+Down=back|Hold Sel+Up=menu"
};
