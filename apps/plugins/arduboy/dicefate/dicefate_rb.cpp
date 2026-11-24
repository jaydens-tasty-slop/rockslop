/* Rockbox game description for DICE of FATE. B rolls and confirms, A goes back: SELECT = B, SELECT + DOWN = A. */
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
    "A digital dice roller: choose the dice type and how many to throw.", "",
    "Up/Down/Left/Right: choose dice and amount.", "",
    "Select: roll the dice (B); confirms in menus.", "",
    "Select + Down: back to the menu (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "DICE of FATE",
    "",
    "TEAM a.r.g.: CastPixel, JO3RI (2016).",
    "",
    "https://github.com/Team-ARG-Museum/ID-37-DICE-of-FATE",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "dicefate", "DICE of FATE", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=roll  Sel+Down=back|Hold Sel+Up=menu"
};
