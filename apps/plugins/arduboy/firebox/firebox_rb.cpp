/* Rockbox game description for Firebox (Jonathan Howard). SELECT = B , SELECT + DOWN = A. */
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
    "A fire sandbox: drop objects and set them alight.", "",
    "Up / Down: choose object.  Left / Right: move it.", "",
    "Select (B): drop the object.", "",
    "Select + Down (A): hold to increase fire.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Firebox", "",
    "Jonathan Howard, github.com/jhhoward/Firebox", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "firebox", "Firebox", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=drop Sel+Dn=fire|Sel+Up=menu"
};
