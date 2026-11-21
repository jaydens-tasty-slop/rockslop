/* Rockbox game description for TechnoMonkey. Select = B (jump / choose), Select+Down = A (shoot); Down = pause. */
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
    "A meteor mutated the zoo animals into robot Technoids. Beat the "
    "six bosses with your buster.", "",
    "Left/Right: move; on the map, pick a boss.", "",
    "Select: jump / choose (B).", "",
    "Select + Down: shoot (A); title: start.", "",
    "Down: pause during a fight.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "TechnoMonkey",
    "",
    "Zachariah Falgout (RetrobitCoder), 2018.",
    "",
    "https://github.com/RetrobitCoder/TechnoMonkey @ 266686b",
    "",
    "License: MIT. Banner art by LENZ is not included. The game has no "
    "sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "technomonkey", "TechnoMonkey", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=move  Sel=jump|Hold Sel+Up=menu"
};
