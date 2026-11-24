/* Rockbox game description for Defenduino. B fires, A is the smart bomb,
 * A while B is held is hyperspace. SELECT = B, SELECT + DOWN = A, SELECT held
 * long = A + B. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LONG, AB_B_A | AB_B_B},
    {AB_P_SELECT | AB_P_DOWN, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Defend the humanoids from the alien landers, as in Defender.", "",
    "Directions: fly (Left/Right turn and thrust).", "",
    "Select: fire.", "",
    "Select + Down: smart bomb.", "",
    "Hold Select for 0.7 s: hyperspace.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Defenduino", "",
    "Alex McChessers.", "",
    "https://bitbucket.org/alexmcchessers/defenduino", "",
    "ArdBitmap by Ignacio Vina (Apache-2.0).", "",
    "License: BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "defenduino", "Defenduino", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 700, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=bomb|Hold Sel=warp  Sel+Up=menu"
};
