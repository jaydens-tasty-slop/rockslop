/* Rockbox game description for The Quest for Truth. Select = A (jump, confirm); Sel+Down = B (pepper spray, back); Down = menu; Up = enter doors. */
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
    "Wacky platformer with RPG elements: explore the village, find "
    "clues and items, avoid the enemies.", "",
    "Left/Right: walk. Select: jump. Select+Down: pepper spray (once "
    "found). Down: menu. Up: enter a door.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "The Quest for Truth",
    "",
    "Guillaume Elias.",
    "",
    "https://github.com/GuillaumeElias/TheQuestForTruth @ 961a215",
    "",
    "License: LGPL 3. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "questtruth", "The Quest for Truth", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Lt/Rt=walk  Sel=jump|SelDn=spray  Dn=menu|Up=door|Hold Sel+Up=menu"
};
