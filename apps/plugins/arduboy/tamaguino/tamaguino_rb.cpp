/* Rockbox game description for Tamaguino. Select = A (select), Select+Down = B (back); Up = menu. */
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
    "A virtual-pet game: feed, play with, clean up after and discipline "
    "your dinosaur as it grows.", "",
    "Up: open the menu.", "",
    "Left/Right: move in the menu.", "",
    "Select: confirm (A).", "",
    "Select + Down: back (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tamaguino",
    "",
    "Alojz Jakob; Arduboy port by Keyboard_Camper.",
    "",
    "https://github.com/KeyboardCamper/Tamaguino-AB @ bd4b513",
    "",
    "License: GPL 3. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tamaguino", "Tamaguino", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up=menu  Sel=ok|Hold Sel+Up=menu"
};
