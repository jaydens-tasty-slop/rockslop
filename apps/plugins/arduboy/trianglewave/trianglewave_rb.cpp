/* Rockbox game description for Triangle Wave. Select = A (shoot/start); Select+Down = B (restart). */
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
    "Fly the triangle wave between ceiling and floor and shoot the "
    "sawblades.", "",
    "Hold Up: rise (let go to sink). Select: start / shoot (A).", "",
    "Sel+Down: B (restart after a crash).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Triangle Wave",
    "",
    "SergioFLS, 2020 (first Arduboy game).",
    "",
    "https://github.com/SergioFLS/TriangleWave @ 11f4ac1",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "trianglewave", "Triangle Wave", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up=rise  Sel=shoot|Sel+Dn=B|Hold Sel+Up=menu"
};
