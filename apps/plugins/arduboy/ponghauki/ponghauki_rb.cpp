/* Rockbox game description for PongHauKi. Select = A (confirm), Select+Down = B (help). */
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
    "A two-piece Chinese board game: take turns moving a piece to the "
    "empty point; whoever is blocked loses.", "",
    "Directions: choose a piece or a point.", "",
    "Select: confirm (A).", "",
    "Select + Down: help (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "PongHauKi",
    "",
    "databhor (2018).",
    "",
    "https://github.com/databhor/PongHauKi @ bcbc762",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ponghauki", "PongHauKi", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=OK  Sel+Down=help|Hold Sel+Up=menu"
};
