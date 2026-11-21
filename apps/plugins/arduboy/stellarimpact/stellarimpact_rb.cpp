/* Rockbox game description for Stellar Impact. Select = B (fire; starts), Select+Down = A (bomb). */
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
    "A side-scrolling space shooter: shoot enemies, collect items and "
    "bomb the swarms.", "",
    "Directions: fly.", "",
    "Select: fire (B); starts the game.", "",
    "Select + Down: bomb (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Stellar Impact",
    "",
    "Athene Allen (2018).",
    "",
    "https://github.com/ArduboyCollection/Stellar_Impact @ e56419c",
    "",
    "License: MIT. Effects through the v1 ArduboyTunes shim over "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "stellarimpact", "Stellar Impact", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=bomb|Hold Sel+Up=menu"
};
