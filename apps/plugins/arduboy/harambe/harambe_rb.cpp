/* Rockbox game description for Harambe's Revenge. A and B are equivalent in
 * the game (menus and dash); SELECT = A, SELECT + DOWN = B. */
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
    "Harambe breaks out of the zoo and takes revenge on the tourists.", "",
    "Directions: move.", "",
    "Hold Select while moving: dash.", "",
    "Select: choose in the menus.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Harambe's Revenge", "",
    "Molly Carpenter and Danielle Lapis, for the first Arduboy Game Jam.", "",
    "https://github.com/ArduboyCollection/Harambe-s-Revenge", "",
    "License: zlib.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "harambe", "Harambe's Revenge", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Hold Sel+move=dash|Hold Sel+Up=menu"
};
