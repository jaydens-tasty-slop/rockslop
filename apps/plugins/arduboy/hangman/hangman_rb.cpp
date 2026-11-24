/* Rockbox game description for Hangman. Select = A (choose letter/confirm), Select+Down = B (pause). */
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
    "Guess the word one letter at a time before the hangman is "
    "complete.", "",
    "Directions: choose a letter.", "",
    "Select: pick the letter (A) and confirm.", "",
    "Select + Down: pause (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Hangman",
    "",
    "serisman (2018); word list from github.com/Xethron/Hangman.",
    "",
    "https://github.com/serisman/arduboy-hangman @ da1825f",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "hangman", "Hangman", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=pick  Sel+Down=pause|Hold Sel+Up=menu"
};
