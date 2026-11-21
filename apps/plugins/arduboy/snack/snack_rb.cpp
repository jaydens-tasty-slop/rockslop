/* Rockbox game description for Snack. Select = B (start); directions steer. */
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
    "A snake game: eat the pieces, grow, and avoid your own tail and "
    "the walls.", "",
    "Directions: steer.", "",
    "Select: start (B).", "",
    "Title: Left = sound on, Right = sound off.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Snack",
    "",
    "Shane Powell (2017).",
    "",
    "https://github.com/shane-powell/ArduboyGames @ 23ad3fe (snack)",
    "",
    "License: MIT (no holder named in the licence file). Sound is off "
    "by default; Left/Right on the title toggles it.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "snack", "Snack", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=steer  Sel=start|Hold Sel+Up=menu"
};
