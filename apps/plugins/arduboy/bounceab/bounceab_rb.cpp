/* Rockbox game description for The Bounce. Select = A; Select+Down = B. */
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
    "A bouncing-ball platform game with unlockable levels.", "",
    "Left/Right: move. Up: jump. Select: A. Sel+Down: B.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "The Bounce",
    "",
    "Joshimuz (repository owner; the licence text leaves the copyright "
    "holder blank).",
    "",
    "https://github.com/ArduboyCollection/TheBounceArduboy @ 2fcc65f",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "bounceab", "The Bounce", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=A  Sel+Dn=B|Hold Sel+Up=menu"
};
