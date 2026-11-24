/* Rockbox game description for Fantasy Rampage. Every action key is A or B: SELECT = B (SELECT + DOWN = A, same effect). */
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
    "A fantasy card battle for one or two players, basic or advanced "
    "rules.", "",
    "Left/Right: pick a card. Up/Down: choose in menus. Up in a game "
    "asks to quit.", "",
    "Select: play the card / confirm.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Fantasy Rampage",
    "",
    "TEAM a.r.g.: Trodoss, JO3RI (2017).",
    "",
    "https://github.com/Team-ARG-Museum/ID-48-Fantasy-Rampage",
    "",
    "Code, art and music: MIT licence, (c) TEAM a.r.g. "
    "Music player: ATMlib, BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "fantasyrampage", "Fantasy Rampage", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=ok  Left/Right=card|Hold Sel+Up=menu"
};
