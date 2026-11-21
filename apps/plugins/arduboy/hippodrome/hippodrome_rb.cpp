/* Rockbox game description for Hippodrome. Select = A (select/move), Select+Down = B (back). */
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
    "A single-player chess variant: take every knight to the last row. "
    "Captures are not allowed.", "",
    "Up/Down/Left/Right: pick the next or previous piece, or a "
    "destination.", "",
    "Select: confirm / move (A).", "",
    "Select + Down: back (B).", "",
    "Menu: Left/Right switch the sound off and on.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Hippodrome",
    "",
    "Miguel Riaza Valverde.",
    "",
    "https://github.com/MiguelRiazaValverde/Hippodrome @ 10db095",
    "",
    "License: GPL 3. Music and effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "hippodrome", "Hippodrome", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=pick  Sel=ok|Hold Sel+Up=menu"
};
