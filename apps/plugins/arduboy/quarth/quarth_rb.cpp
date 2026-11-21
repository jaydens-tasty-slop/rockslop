/* Rockbox game description for Space Quarth. Select = B (fire); Select+Down = A (pause). */
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
    "A Quarth-style puzzle shooter: fire blocks at the descending block "
    "groups to fill gaps and make them vanish.", "",
    "Left/Right: move the ship. Select (B): fire. Sel+Down (A): pause. "
    "Up: speed up.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Space Quarth",
    "",
    "Schuemi, 2017.",
    "",
    "https://github.com/ArduboyCollection/SpaceQuarth @ e76e561",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "quarth", "Space Quarth", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=fire|Sel+Dn=pause|Hold Sel+Up=menu"
};
