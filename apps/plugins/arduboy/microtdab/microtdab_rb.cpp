/* Rockbox game description for MicroTD. Select = A (confirm); Select+Down = B (cancel). The A+B speed-up chord is not reachable. */
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
    "A tower defense game: build towers along the path and stop the "
    "creeps over successive waves.", "",
    "Directions: move the cursor. Select (A): confirm and build. "
    "Sel+Down (B): cancel.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "MicroTD",
    "",
    "Miloslav Ciz (drummyfish), 2018.",
    "",
    "https://gitlab.com/drummyfish/Arduboy_TD @ 0c8958f",
    "",
    "License: CC0 (public domain). Beeps through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "microtdab", "MicroTD", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=cursor  Sel=OK|Sel+Dn=cancel|Hold Sel+Up=menu"
};
