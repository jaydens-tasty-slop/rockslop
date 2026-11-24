/* Rockbox game description for Bangi (Ignacio Vina). SELECT = B , SELECT + DOWN = A. */
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
    "Destroy all the balloons with your harpoon.", "",
    "Left/Right: move.  Select (B): fire.  Select + Down (A): menu choice / special.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Bangi", "",
    "Ignacio Vina (igvina), github.com/igvina/Bangi", "",
    "License: Apache-2.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "bangi", "Bangi", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire Sel+Dn=A|Sel+Up=menu"
};
