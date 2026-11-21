/* Rockbox game description for Helmets and Hordes. B jumps, A attacks: SELECT = B, SELECT + DOWN = A. Up, Right or Down pause, Left leaves to the title. */
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
    "Helena fights wave after wave of monsters; pick up helmets for new "
    "powers.", "",
    "Select: jump (B); confirms in menus.", "",
    "Select + Down: attack (A). With the thief helmet, attack twice.", "",
    "Up or Right: pause the game. Left: back to the title screen.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Helmets and Hordes",
    "",
    "TEAM a.r.g. (2017 - 2018).",
    "",
    "https://github.com/Team-ARG-Museum/ID-47-Helmets-Hordes",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "helmetshordes", "Helmets and Hordes", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=attack|Hold Sel+Up=menu"
};
