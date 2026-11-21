/* Rockbox game description for Athletic Boy. Select = B (play); Select+Down = A (jump); Up jumps too. */
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
    "Walk from the left tree to the right tree in 20 stages without "
    "being hit or drowning.", "",
    "Left/Right: walk. Up: jump. Down: bend. Select: start (B).", "",
    "You start with 5 lives and a time limit.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Athletic Boy",
    "",
    "Frank van de Ven, 2024. Based on Athletic Land (MSX).",
    "",
    "https://github.com/frankvandeven/AthleticBoy @ 7025f04",
    "",
    "License: MIT. No sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "athleticboy", "Athletic Boy", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Up=jump  Sel=start|Hold Sel+Up=menu"
};
