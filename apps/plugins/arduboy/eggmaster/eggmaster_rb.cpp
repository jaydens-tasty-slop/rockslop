/* Rockbox game description for Egg Master Pro 2020. Select = A (select); Select+Down = B (back/reset). Options (Left+B) is Left+Sel+Down. */
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
    "An egg timer with configurable alarm and egg program tables.", "",
    "Up/Down: move cursor. Select: choose (A). Sel+Down: back / reset "
    "(B).", "",
    "Options: hold Left with Sel+Down.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Egg Master Pro 2020",
    "",
    "Bert van't Veer, 2019. Entry for Arduboy Game Jam 4.",
    "",
    "https://github.com/BertVeer/EggMasterPro2020 @ 4e6dccc",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "eggmaster", "Egg Master Pro 2020", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up/Dn=move  Sel=OK|Sel+Dn=back|Hold Sel+Up=menu"
};
