/* Rockbox game description for Roshambo. Default mapping: SELECT = A. */
#include <Arduino.h>

static const char *const help[] = {
    "Rock, paper, scissors against the computer, in a console window.", "",
    "Up/Down: choose rock, paper or scissors.", "",
    "Select: confirm.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Roshambo", "",
    "CDRXavier.", "",
    "https://github.com/CDRXavier/Roshambo", "",
    "License: GPL-2.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "roshambo", "Roshambo", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up/Down=choose  Sel=ok|Hold Sel+Up=menu"
};
