/* Rockbox game description for i4arduboy (Submarine shmup). Select = A (torpedo); Select+Down = B (torpedo/cancel). The A+B+Left return-to-title chord is not reachable; use the pause menu. */
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
    "A submarine shmup: guns fire automatically; launch torpedoes "
    "against big enemies. Enemies also lurk outside the visible area.", "",
    "Directions: move. Select (A) or Sel+Down (B): launch torpedo.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "i4arduboy (Submarine shmup)",
    "",
    "Planet Marimo, 2015. Arduboy port of the shmup i-20000.",
    "",
    "https://github.com/ArduboyCollection/i4arduboy @ 60e156f",
    "",
    "License: MIT. Music and effects through the Playtune shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "submarine", "i4arduboy (Submarine shmup)", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=torpedo|Hold Sel+Up=menu"
};
