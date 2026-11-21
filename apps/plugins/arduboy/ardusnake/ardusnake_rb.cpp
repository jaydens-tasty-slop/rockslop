/* Rockbox game description for ArduSnake. Select = B (confirm/pause), Select+Down = A. */
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
    "Classic snake for one to three players on one screen. Eat food to "
    "grow; do not hit walls or snakes.", "",
    "Directions: steer the snake (player 1).", "",
    "Select: choose a title option or pause (B).", "",
    "Select + Down: player 2 turn / pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "ArduSnake",
    "",
    "Alec Tutin (chootin).",
    "",
    "https://github.com/chootin/ardusnake @ cb74b97",
    "",
    "License: GPL 3. Beeps play through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardusnake", "ArduSnake", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=steer  Sel=pick|Hold Sel+Up=menu"
};
