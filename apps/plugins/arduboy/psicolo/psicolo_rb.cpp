/* Rockbox game description for Psi Colo. The OBONO games confirm with B and use
 * A for cancel / menu, so: SELECT = B, SELECT + DOWN = A. */
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
    "A puzzle game with dice on the 5x7 grid field.", "",
    "Directions: move.", "",
    "Select: the B button (confirm, main action).", "",
    "Select + Down: the A button (cancel, menu, sound on/off in menus).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Psi Colo", "",
    "OBONO (2016-2026).", "",
    "https://github.com/obono/ArduboyWorks", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "psicolo", "Psi Colo", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=B (OK)  Sel+Down=A|Hold Sel+Up=menu"
};
