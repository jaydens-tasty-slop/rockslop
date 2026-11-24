/* Rockbox game description for Battle for Zuthsrah. Select = A (weapon 1), Select+Down = B (weapon 2); either restarts after game over. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A space shoot-em-up: pilot Belknar and blast the waves of enemies. "
    "Your kill count is shown at the top.", "",
    "Directions: fly.", "",
    "Select: fire weapon A.", "",
    "Select + Down: fire weapon B.", "",
    "After game over, either fire button restarts (upstream: A+B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Battle for Zuthsrah",
    "",
    "Mre64 (2020).",
    "",
    "https://github.com/Mre64/BattleForZuthsrah @ 4433a4e",
    "",
    "License: MIT (template text; the repository names no copyright "
    "holder). Playtune effects.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "zuthsrah", "Battle for Zuthsrah", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=fly  Sel=fire|Hold Sel+Up=menu"
};
