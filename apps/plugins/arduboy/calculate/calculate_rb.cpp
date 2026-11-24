/* Rockbox game description for Calculate. Select = A (start), Select+Down = B (confirm the answer; Up also confirms). */
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
    "Pick the correct answer to the sum before the clock runs out.", "",
    "Left/Right: choose an answer.", "",
    "Up or Select + Down: confirm (B).", "",
    "Select: start (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Calculate",
    "",
    "Frank van de Ven (2025); idea after B Math by ABA Games.",
    "",
    "https://github.com/FrankVanDeVen/Calculate @ cda5dc7",
    "",
    "License: MIT. No sound upstream. Scores are kept in the per-game "
    "save file.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "calculate", "Calculate", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up=confirm  Sel=start|Hold Sel+Up=menu"
};
