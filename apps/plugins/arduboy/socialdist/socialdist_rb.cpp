/* Rockbox game description for The Social Distance Game. Select = A (pause/start). */
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
    "Keep out of people's way as long as you can!", "",
    "Left/Right: move. Select: start and pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "The Social Distance Game",
    "",
    "Marcus Sanatan (msanatan), 2020.",
    "",
    "https://github.com/msanatan/TheSocialDistanceGame @ 025e866",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "socialdist", "The Social Distance Game", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Left/Right=move  Sel=start/pause|Hold Sel+Up=menu"
};
