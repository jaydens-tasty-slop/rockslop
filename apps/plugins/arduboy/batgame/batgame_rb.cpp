/* Rockbox game description for BAT-GAME. Select = A (flap), Select+Down = B (shoot sonic). */
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
    "A side-scrolling shooter: fly the bat and blast what comes at you.", "",
    "Left/Right: speed; Down: dive.", "",
    "Select: flap (A).", "",
    "Select + Down: shoot a sonic wave (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "BAT-GAME",
    "",
    "Quickdrawkid (2020).",
    "",
    "https://github.com/Quickdrawkid/BAT-GAME @ 19789af",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "batgame", "BAT-GAME", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=flap  Sel+Down=shoot|Hold Sel+Up=menu"
};
