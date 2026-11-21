/* Rockbox game description for Sirene. B fires (hold to charge magic), A switches weapon: SELECT = B, SELECT + DOWN = A. */
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
    "A mermaid shooter: swim through the stages, shoot the sea "
    "creatures and collect power-ups.", "",
    "Directions: swim.", "",
    "Select: fire (B); hold to charge the magic weapon. Confirms in "
    "menus.", "",
    "Select + Down: switch weapon / pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Sirene",
    "",
    "TEAM a.r.g.: JO3RI, Justin Cyr (2016).",
    "",
    "https://github.com/Team-ARG-Museum/ID-42-Sirene",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "sirene", "Sirene", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=weapon|Hold Sel+Up=menu"
};
