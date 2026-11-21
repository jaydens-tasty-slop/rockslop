/* Rockbox game description for Arduwars. B confirms (SELECT), A cancels/scrolls fast (SELECT+DOWN). */
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
    "A turn-based strategy game after Advance Wars: capture buildings, "
    "build units and take the enemy HQ.", "",
    "Directions: move the cursor.", "",
    "Select: confirm / select a unit (B).", "",
    "Select + Down: cancel (A); hold it to scroll the map faster.", "",
    "Play MULTIPLAYER (two players pass the device). NEW GAME is an "
    "unfinished stub upstream.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Arduwars",
    "",
    "Botond Kis (2018).",
    "",
    "https://github.com/yinkou/Arduwars @ 9cd6ce8",
    "",
    "License: BSD 3-Clause (SimpleArduinoUtilities: Apache 2.0). The "
    "game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arduwars", "Arduwars", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=OK  Sel+Down=cancel|Hold Sel+Up=menu"
};
