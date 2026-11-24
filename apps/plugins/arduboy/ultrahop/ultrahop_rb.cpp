/* Rockbox game description for Ultra Hop. Select = B (jump); Select+Down = A (run). */
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
    "Side-scrolling platformer: 4 worlds and 13 levels.", "",
    "Left/Right: move. Select: jump (hold for higher). Sel+Down: run.", "",
    "Up: pause. Progress is saved.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ultra Hop",
    "",
    "rolohaun, 2026.",
    "",
    "https://github.com/rolohaun/ultra-hop @ 19c4801",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ultrahop", "Ultra Hop", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=jump|Sel+Dn=run|Hold Sel+Up=menu"
};
