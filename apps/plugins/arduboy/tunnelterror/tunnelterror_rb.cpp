/* Rockbox game description for Tunnel Terror. Select = A (confirm / fight), Select+Down = B (flee / alternate action). */
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
    "A tunnel RPG: find the three gold chalices, then beat Zarc.", "",
    "Directions: walk the corridors; Up opens doors.", "",
    "Select: confirm / fight (A).", "",
    "Select + Down: the other choice in battles (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tunnel Terror",
    "",
    "Tom and Jamie Millichamp (2020).",
    "",
    "https://github.com/Tomsk666/arduboy-terror-tunnels @ 01d5bb9",
    "",
    "License: GPL 3. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tunnelterror", "Tunnel Terror", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
