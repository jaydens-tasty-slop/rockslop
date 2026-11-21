/* Rockbox game description for Street Racer. Select = A (confirm / horn), Select+Down = B (back). */
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
    "Dodge traffic on a busy road; levels get harder.", "",
    "Left/Right: steer.", "",
    "Up/Down: speed.", "",
    "Select: confirm (A).", "",
    "Select + Down: back (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Street Racer",
    "",
    "marggines (2023).",
    "",
    "https://github.com/marggines/Street-Racer @ 84e86e6",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "streetracer", "Street Racer", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up/Down=speed  Sel=confirm|Hold Sel+Up=menu"
};
