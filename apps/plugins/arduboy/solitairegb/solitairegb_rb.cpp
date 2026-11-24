/* Rockbox game description for Solitaire (Gamebuino). Select = A (pick up/place/draw); Select+Down = B (auto-move to foundation); Select+Left = C (pause menu). */
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
    "Klondike solitaire, one or three cards per draw.", "",
    "Directions: move the cursor or the carried cards. Select (A): pick "
    "up, place, draw. Sel+Down (B): move to foundation. Sel+Left (C): "
    "pause menu with undo and statistics.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Solitaire (Gamebuino)",
    "",
    "Andy O'Neill, 2017. Gamebuino Klondike solitaire.",
    "",
    "https://github.com/ArduboyCollection/gamebuino-solitaire @ 80f3f76",
    "",
    "License: MIT. Gamebuino API provided by the Gamebuino shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "solitairegb", "Solitaire (Gamebuino)", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=cursor  Sel=pick|Sel+Dn=found|Sel+Lt=menu|Hold Sel+Up=menu"
};
