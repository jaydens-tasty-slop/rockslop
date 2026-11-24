/* Rockbox game description for Shroom Knight. */
#include <Arduino.h>

/* Jump (A) and attack (B) must work while running, and Left/Right cannot be
 * combined with each other, so: UP = jump (A), SELECT = attack (B),
 * SELECT + DOWN = look up (the original's Up), DOWN = look down. */
static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_UP},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_A},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Escape the testing facility.", "",
    "Left/Right: run. Up: jump (hold for height; press twice for the "
    "double jump skill).", "",
    "Select: attack (sword, later the gun).", "",
    "Down: look down. Select + Down: look up.", "",
    "Hold Select + Up for 1 s: pause menu (Select + Up is also attack "
    "while jumping, hence the longer hold).",
    NULL
};

static const char *const credits[] = {
    "Shroom Knight", "",
    "Onebit Productions & Filmote, Matthew Bryan (2024).", "",
    "https://github.com/TropicAnthropic/Shroom-Knight", "",
    "License: GPL-3.0 (code and graphics as published in that repository). "
    "Tinyfont: (c) 2017 Botond Kis, BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "shroom", "Shroom Knight", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 1000, 0, 0, 0, NULL, 0,
    "Up=jump  Sel=attack|Hold Sel+Up=menu"
};
