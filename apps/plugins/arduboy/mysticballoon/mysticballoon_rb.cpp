/* Rockbox game description for Mystic Balloon. B jumps (hold it in the air
 * to float on the balloon) and A sucks in the wind, so: SELECT = B,
 * SELECT + DOWN = A. */
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
    "Guide the wizard Pip through 39 levels: find the key, collect the "
    "coins and reach the exit. Your balloons are your health.", "",
    "Left/Right: walk. Up/Down: look around.", "",
    "Select: jump (B). Hold it in the air to float on the balloon.", "",
    "Select + Down: suck in the wind (A, hold). Confirms in menus too.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Mystic Balloon", "",
    "TEAM a.r.g.: Gaveno, CastPixel, JO3RI, Martian220 (2016 - 2018).", "",
    "https://github.com/Team-ARG-Museum/ID-34-Mystic-Balloon", "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "mysticballoon", "Mystic Balloon", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=suck|Hold Sel+Up=menu"
};
