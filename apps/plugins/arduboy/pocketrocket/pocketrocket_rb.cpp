/* Rockbox game description for Pocket Rocket. SELECT = A (thrust),
 * SELECT + DOWN = B (parachute). */
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
    "Fly a tiny rocket across 100 levels and set it down slow and straight on the pad.", "",
    "Hold Select: thrust.  Left / Right: tilt.", "",
    "Select + Down: parachute (empty tank, above 100 m).", "",
    "Hold Select on the title screen to launch.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pocket Rocket", "",
    "jaychugstein, github.com/jaychugstein/Pocket-Rocket", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "pocketrocket", "Pocket Rocket", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, "pocketrocket/fxdata.bin", 0,
    "Sel=thrust  Sel+Down=chute|Sel+Up=menu"
};
