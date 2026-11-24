/* Rockbox game description for Super Crate Buino. Select = A (shoot); Up = B (jump, hold for higher); Sel+Down = B as well. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_B},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Platform shooter inspired by Super Crate Box: collect crates for "
    "new weapons and survive the enemy waves.", "",
    "Left/Right: run. Up: jump (hold for higher). Select: shoot.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Super Crate Buino",
    "",
    "Aurelien Rodot, 2014 (Gamebuino original). Arduboy port by Dan "
    "O'Shea.",
    "",
    "https://github.com/uXeBoy/Super-Crate-Buino @ 6441161",
    "",
    "License: LGPL 3 or later. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "supercratebuino", "Super Crate Buino", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Lt/Rt=run  Up=jump  Sel=shoot|Hold Sel+Up=menu"
};
