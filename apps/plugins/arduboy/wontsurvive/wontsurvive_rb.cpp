/* Rockbox game description for You Won't Survive. Select = A (easy / retry); Select+Down = B (hard / menu). The LED effects are no-ops. */
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
    "A simple arcade game: dodge the enemy and survive.", "",
    "Up/Down: move. Select (A): easy mode / retry. Sel+Down (B): hard "
    "mode / menu.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "You Won't Survive",
    "",
    "Andre Hellwig, 2016.",
    "",
    "https://github.com/ArduboyCollection/YouWontSurvive @ c6f489e",
    "",
    "License: MIT (named in the source header). Sounds through the "
    "Arduboy v1 tunes shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "wontsurvive", "You Won't Survive", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up/Dn=move  Sel=easy|Sel+Dn=hard|Hold Sel+Up=menu"
};
