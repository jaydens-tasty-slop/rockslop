/* Rockbox game description for Virus LQP-79. B = stand and shoot (and confirm), A = run and gun: SELECT = B, SELECT + DOWN = A. */
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
    "Fight through the infected laboratory: shoot the zombies, collect "
    "pickups and open the doors.", "",
    "Left/Right: walk. Up/Down: look up/down.", "",
    "Select: stand and shoot (B, hold). Confirms in menus.", "",
    "Select + Down: run and gun (A, hold). Careful, the gun "
    "overheats.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Virus LQP-79",
    "",
    "TEAM a.r.g.: Fuopy, JO3RI, STG, CastPixel, Justin Cyr (2016).",
    "",
    "https://github.com/Team-ARG-Museum/ID-40-VIRUS-LQP-79",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "virus", "Virus LQP-79", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=shoot  Sel+Down=run|Hold Sel+Up=menu"
};
