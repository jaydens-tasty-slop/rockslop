/* Rockbox game description for Tunnel Fighter. Select = A (fire / start), Select+Down = B (also fires). */
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
    "Fly a fighter down a tunnel, shooting enemy jets.", "",
    "Directions: fly.", "",
    "Select: fire / start (A).", "",
    "Select + Down: fire (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tunnel Fighter",
    "",
    "Matthew Bryan (2024).",
    "",
    "https://github.com/TropicAnthropic/tunnelFighter @ eec0991",
    "",
    "License: GPL 3. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tunnelfighter", "Tunnel Fighter", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=fire|Hold Sel+Up=menu"
};
