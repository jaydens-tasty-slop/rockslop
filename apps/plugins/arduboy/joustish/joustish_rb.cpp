/* Rockbox game description for Joustish. Select = A (flap), Select+Down = B (also flaps). */
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
    "A Joust-style arena: flap to stay aloft and land on enemy riders "
    "from above.", "",
    "Left/Right: steer.", "",
    "Select: flap (A).", "",
    "Select + Down: flap (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Joustish",
    "",
    "wuuff (2017).",
    "",
    "https://github.com/wuuff/joustish @ 18d3988",
    "",
    "License: GPL 3. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "joustish", "Joustish", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=flap  Left/Right=steer|Hold Sel+Up=menu"
};
