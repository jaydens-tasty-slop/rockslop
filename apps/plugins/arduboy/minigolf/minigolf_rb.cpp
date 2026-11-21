/* Rockbox game description for Mini Golf. SELECT = B , SELECT + DOWN = A. */
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
    "Sink the ball in as few strokes as you can on a series of top-down courses.", "",
    "Left / Right: aim.", "",
    "Select (B): hold to build power, release to hit.", "",
    "Select + Down (A): confirm / menu actions.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Mini Golf", "",
    "BennettB123, https://github.com/BennettB123/Arduboy-2DMiniGolf", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "minigolf", "Mini Golf", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, "minigolf/fxdata.bin", 0,
    "Sel=B  Sel+Down=A|Sel+Up=menu"
};
