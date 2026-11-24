/* Rockbox game description for LATE. Select = A (choose), Select+Down = B (pause menu / back). */
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
    "Little Arduboy Timekiller Endeavour: three games in one: Blocks "
    "Arcade (falling blocks), 1010 (fit pieces on a grid) and a bridge- "
    "building runner.", "",
    "Directions: move; Up/Down choose in menus.", "",
    "Select: A (choose / rotate).", "",
    "Select + Down: B (pause menu, saves the game).", "",
    "Menu: hold Left while pressing A to reset.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "LATE",
    "",
    "Vladimir Borisov (core1024), 2018.",
    "",
    "https://github.com/core1024/LATE @ 643440b",
    "",
    "License: MIT. Upstream has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "late", "LATE", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=A|Hold Sel+Up=menu"
};
