/* Rockbox game description for Coin Rush. Select = A (start/jump); Select+Down = B (save high score). */
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
    "Steer the car, grab the coins and dodge the bouncing enemies for "
    "as long as you can.", "",
    "Directions: move.", "",
    "Select: start / turn (A).", "",
    "Select + Down: save the high score (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Coin Rush",
    "",
    "ArdFlamingo (2022).",
    "",
    "https://github.com/Pharap/ArdFlamingo-Arduboy-Games @ 5de05a7",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "coinrush", "Coin Rush", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=start|Hold Sel+Up=menu"
};
