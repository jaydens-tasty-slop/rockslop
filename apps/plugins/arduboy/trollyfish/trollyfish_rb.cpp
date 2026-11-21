/* Rockbox game description for Trolly Fish. B fires, A pauses, so
 * SELECT = B, SELECT + DOWN = A. */
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
    "Swim the Trolly Fish through the shopping-cart littered sea: avoid "
    "the enemies, grab the power-ups and shoot back.", "",
    "Up/Down: swim.", "",
    "Select: fire bubbles or an electric shock (B); confirms in menus.", "",
    "Select + Down: pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Trolly Fish", "",
    "TEAM a.r.g.: Gaveno, JO3RI, Justin Cry (2016).", "",
    "https://github.com/Team-ARG-Museum/ID-36-Trolly-Fish", "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "trollyfish", "Trolly Fish", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=pause|Hold Sel+Up=menu"
};
