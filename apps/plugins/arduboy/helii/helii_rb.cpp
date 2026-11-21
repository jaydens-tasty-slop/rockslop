/* Rockbox game description for Helii. Select = B (thrust, hold), Select+Down = A. */
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
    "Fly the helicopter through the cave without touching the walls.", "",
    "Select: thrust (B, hold); starts the game.", "",
    "Select + Down: pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Helii",
    "",
    "Stephen Eisenhauer (2016).",
    "",
    "https://github.com/ArduboyCollection/Helii-Arduboy @ f6f9a8f",
    "",
    "License: MIT. Effects through the v1 tunes shim over ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "helii", "Helii", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=thrust|Hold Sel+Up=menu"
};
