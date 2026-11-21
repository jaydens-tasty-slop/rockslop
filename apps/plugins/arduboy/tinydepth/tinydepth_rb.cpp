/* Rockbox game description for Tiny Depth. Select = A (drop a mine to the left / menu choice), Select+Down = B (drop to the other side). */
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
    "Steer a ship over the sea and drop depth charges on the submarines "
    "below.", "",
    "Left/Right: sail.", "",
    "Select: drop a charge (A); menu choice.", "",
    "Select + Down: drop the other charge (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiny Depth",
    "",
    "akkera102 (2017).",
    "",
    "https://github.com/akkera102/02_tiny_depth @ 7cc8c8e",
    "",
    "License: GPL 2 (the repository carries the GPL text). Effects "
    "through ArduboyTones. The upstream background tune is borrowed "
    "MIDI data without a licence, so it is left out; the BGM menu "
    "switch does nothing here.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tinydepth", "Tiny Depth", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=drop  Sel+Down=other|Hold Sel+Up=menu"
};
