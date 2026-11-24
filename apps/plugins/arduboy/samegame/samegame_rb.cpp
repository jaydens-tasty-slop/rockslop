/* Rockbox game description for SameGame. The game confirms with B and uses
 * A for the menu and cancel, so: SELECT = B, SELECT + DOWN = A. */
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
    "A tile-matching puzzle: remove groups of adjoining blocks of the same "
    "type. Bigger groups score more; clear the board for a bonus.", "",
    "Directions: move the cursor.", "",
    "Select: remove the group under the cursor (the B button), confirm in menus.", "",
    "Select + Down: the A button (game menu: undo, restart; sound on/off).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "SameGame", "",
    "OBONO (2016-2026), OBN-Y11.", "",
    "https://github.com/obono/ArduboyWorks", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "samegame", "SameGame", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=OK  Sel+Down=options|Hold Sel+Up=menu"
};
