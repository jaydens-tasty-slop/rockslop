/* Rockbox game description for Tower Of Pain. Walk with the directions; A opens the pause menu, B confirms: SELECT = A, SELECT + DOWN = B. */
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
    "A tiny roguelike: climb the tower, fight monsters, find keys and "
    "survive as many floors as you can.", "",
    "Directions: walk and attack by walking into monsters; in menus "
    "Left/Right also confirm or go back.", "",
    "Select: pause menu / back (A).", "",
    "Select + Down: confirm (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tower Of Pain",
    "",
    "Fran J. Alvarez (2021).",
    "",
    "https://github.com/franalvarez21/TowerOfPain",
    "",
    "License: BSD-3-Clause. Sound starts switched off: turn it on in "
    "the pause menu.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "towerofpain", "Tower Of Pain", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=pause|Hold Sel+Up=menu"
};
