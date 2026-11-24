/* Rockbox game description for JezzBall. Select = A (grow wall); Select+Down = B (switch direction). BTN_C (menu) is unreachable. */
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
    "Wall in the bouncing balls: grow walls to cut the board into "
    "smaller areas and claim most of it without a ball hitting a "
    "growing wall.", "",
    "Directions: move the cursor. Select (A): build a wall. Sel+Down "
    "(B): switch wall direction.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "JezzBall",
    "",
    "Thomas Pietrzak, Gamebuino tribute to JezzBall.",
    "",
    "https://github.com/ArduboyCollection/gamebuino-jezzball @ d51e949",
    "",
    "License: GPL 3. Gamebuino API provided by the Gamebuino shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "jezzball", "JezzBall", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=cursor  Sel=wall|Sel+Dn=turn|Hold Sel+Up=menu"
};
