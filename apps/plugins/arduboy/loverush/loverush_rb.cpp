/* Rockbox game description for LoveRush. Select = A (shoot), Select+Down = B (pause). */
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
    "An endless shoot-em-up: catch hearts, dodge the angry faces.", "",
    "Directions: fly.", "",
    "Select: shoot (A).", "",
    "Select + Down: pause (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "LoveRush",
    "",
    "Stephane Cote (vampirics), 2018.",
    "",
    "https://github.com/vampirics/LoveRush @ 993a2db",
    "",
    "Code: MIT. Graphics: CC BY-NC-SA 4.0 (non-commercial use only); "
    "both licence texts are on the card. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "loverush", "LoveRush", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=shoot  Sel+Down=pause|Hold Sel+Up=menu"
};
