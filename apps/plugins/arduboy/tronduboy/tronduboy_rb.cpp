/* Rockbox game description for Tronduboy. Directions steer; Select = B (start). */
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
    "A light-cycle game: steer your trail and stay off the walls and "
    "your own trail for as long as you can.", "",
    "Directions: steer.", "",
    "Select: start / restart (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tronduboy",
    "",
    "Simon Prickett (2016).",
    "",
    "https://github.com/simonprickett/tronduboy @ 66312d8",
    "",
    "License: MIT. Effects through the v1 tunes shim over ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tronduboy", "Tronduboy", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=steer  Sel=start|Hold Sel+Up=menu"
};
