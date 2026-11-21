/* Rockbox game description for Armageddon. Select = A (left launcher); Select+Down = B (right launcher). BTN_C (menu) is unreachable; use the pause menu. */
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
    "Defend your cities from the incoming missiles: launch counter- "
    "missiles and detonate them in the enemies' path.", "",
    "Directions: move the crosshair. Select (A) / Sel+Down (B): launch "
    "from a silo.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Armageddon",
    "",
    "wuuff (repository author); a Gamebuino game inspired by Missile "
    "Command.",
    "",
    "https://github.com/ArduboyCollection/armageddon @ c9c471d",
    "",
    "License: GPL 3. Gamebuino API provided by the Gamebuino shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "armageddon", "Armageddon", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=aim  Sel=A  Sel+Dn=B|Hold Sel+Up=menu"
};
