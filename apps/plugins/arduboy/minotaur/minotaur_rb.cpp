/* Rockbox game description for Minotaur. Select = B (select / sword), Select+Down = A, Select+Left = A+B (back to the menu). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LEFT, AB_B_A | AB_B_B},
    {AB_P_SELECT | AB_P_DOWN, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Retell the myth: Theseus must get through the labyrinth and beat "
    "the Minotaur.", "",
    "Directions: walk.", "",
    "Select: menu choice / draw or sheathe the sword (B).", "",
    "Select + Down: A.", "",
    "Select + Left: back to the menu (A+B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Minotaur",
    "",
    "Maxime Hanicotte (2025).",
    "",
    "https://github.com/ideometre/Minotaur @ 28a5715",
    "",
    "License: Unlicense (public domain). Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "minotaur", "Minotaur", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=sword  Sel+Left=menu|Hold Sel+Up=menu"
};
