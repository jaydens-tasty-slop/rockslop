/* Rockbox game description for Quadfall. Select = B (rotate cw), Select+Down = A (rotate ccw); Down = soft drop. */
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
    "A falling-block puzzle: complete lines to clear them. Speed rises "
    "with the level.", "",
    "Left/Right: move. Down: soft drop. Up: hard drop.", "",
    "Select: rotate clockwise (B).", "",
    "Select + Down: rotate counter-clockwise (A); on the title, start.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Quadfall",
    "",
    "stonedev-app (2026).",
    "",
    "https://github.com/stonedev-app/Quadfall @ 1f81cb7",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "quadfall", "Quadfall", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=move  Sel=rotate|Hold Sel+Up=menu"
};
