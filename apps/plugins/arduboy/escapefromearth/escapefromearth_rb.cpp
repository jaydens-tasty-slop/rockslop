/* Rockbox game description for Escape From Earth. SELECT = B (propel), SELECT + DOWN
 * = A (spin). */
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
    "Fly Glorb's saucer through one long course into the mothership. Touch anything and it is over.", "",
    "Left / Right: tilt the saucer.", "",
    "Hold Select: propel (tap to hover).", "",
    "Select + Down: spin a quarter turn.", "",
    "After a crash: Select tries again, Select + Down goes home.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Escape From Earth", "",
    "jaychugstein, github.com/jaychugstein/Escape-From-Earth", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "escapefromearth", "Escape From Earth", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, "escapefromearth/fxdata.bin", 0,
    "Sel=thrust  Sel+Down=spin|Sel+Up=menu"
};
