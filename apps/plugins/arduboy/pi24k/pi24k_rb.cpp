/* Rockbox game description for Pi 24k. The OBONO games confirm with B and use
 * A for cancel / menu, so: SELECT = B, SELECT + DOWN = A. */
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
    "Browse the digits of pi (24,576 of them).", "",
    "Left/Right: move through digits (hold to go faster).", "",
    "Up: LED sparkle on/off. Down: sound on/off.", "",
    "Select: the B button (play/stop auto scroll).", "",
    "Select + Down: the A button (credits, when stopped at the start).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pi 24k", "",
    "OBONO (2016-2026).", "",
    "https://github.com/obono/ArduboyWorks", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "pi24k", "Pi 24k", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=B (OK)  Sel+Down=A|Hold Sel+Up=menu"
};
