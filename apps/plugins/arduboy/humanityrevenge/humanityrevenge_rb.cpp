/* Rockbox game description for Humanity Revenge DC. Select = A (fire), Select+Down = B (switch weapon), Select+Left = A+B (bomb). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LEFT, AB_B_A | AB_B_B},
    {AB_P_SELECT | AB_P_DOWN, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A vertical space shooter: choose a ship, collect power-ups, beat "
    "the bosses.", "",
    "Directions: fly.", "",
    "Select: fire (A).", "",
    "Select + Down: switch weapon (B).", "",
    "Select + Left: bomb (A+B); this also moves left.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Humanity Revenge DC",
    "",
    "giangregorio (2017).",
    "",
    "https://github.com/ArduboyCollection/Humanity_Revenge_DC @ 9730bf5",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "humanityrevenge", "Humanity Revenge DC", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=weapon|Sel+Left=bomb|Hold Sel+Up=menu"
};
