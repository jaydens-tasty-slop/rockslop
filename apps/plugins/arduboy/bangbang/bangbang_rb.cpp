/* Rockbox game description for Bang! Bang!. Select = A (fire/confirm); Select+Down = B. */
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
    "Two players take turns lobbing cannon shots over random terrain "
    "until one cannon is hit.", "",
    "Up/Down: aim angle. Left/Right: power. Select (A): fire.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Bang! Bang!",
    "",
    "Thomas Pietrzak, Gamebuino implementation of Bang! Bang!.",
    "",
    "https://github.com/ArduboyCollection/gamebuino-bangbang @ cfef988",
    "",
    "License: GPL 3. Gamebuino API provided by the Gamebuino shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "bangbang", "Bang! Bang!", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=aim/power  Sel=fire|Hold Sel+Up=menu"
};
