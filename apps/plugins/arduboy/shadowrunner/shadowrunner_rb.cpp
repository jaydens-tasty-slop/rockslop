/* Rockbox game description for Shadow Runner. B jumps, A or Left ducks;
 * Up, Down or Right pause. SELECT = B, SELECT + DOWN = A. */
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
    "Run through the shadows: jump the obstacles and duck under the "
    "birds for as long as you can.", "",
    "Select: jump (B); confirms in menus.", "",
    "Left or Select + Down: duck.", "",
    "Right or Down: pause the game.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Shadow Runner", "",
    "TEAM a.r.g. (2016).", "",
    "https://github.com/Team-ARG-Museum/ID-15-Shadow-Runner", "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "shadowrunner", "Shadow Runner", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Left=duck|Hold Sel+Up=menu"
};
