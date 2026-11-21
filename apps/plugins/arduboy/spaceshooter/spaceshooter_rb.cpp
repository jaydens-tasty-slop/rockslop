/* Rockbox game description for Space Shooter. Select = B (fire/start); Left/Right move. */
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
    "A small space shooter: slide along the bottom and shoot the "
    "descending ships.", "",
    "Left/Right: move.", "",
    "Select: fire / start (B).", "",
    "Title: Left = sound on, Right = sound off.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Space Shooter",
    "",
    "Shane Powell (2017).",
    "",
    "https://github.com/shane-powell/ArduboyGames @ 23ad3fe (space)",
    "",
    "License: MIT (no holder named in the licence file). Sound is off "
    "by default; Left/Right on the title toggles it.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "spaceshooter", "Space Shooter", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=move  Sel=fire|Hold Sel+Up=menu"
};
