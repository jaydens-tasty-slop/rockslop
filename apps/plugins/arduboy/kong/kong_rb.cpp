/* Rockbox game description for Kong. Default key mapping: SELECT = A
 * (jump), SELECT + DOWN = B (pause, hard mode on the title screen). */
#include <Arduino.h>

static const char *const help[] = {
    "A remake of the Game & Watch Donkey Kong: climb the platforms, jump "
    "the barrels and reach the crane to drop Kong.", "",
    "Left/Right: run. Up/Down: climb.", "",
    "Select: jump (also starts an easy game on the title screen).", "",
    "Select + Down: pause (starts a hard game on the title screen).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Kong", "",
    "Press Play On Tape, Filmote and contributors (2019).", "",
    "https://github.com/Press-Play-On-Tape/Kong", "",
    "License: BSD-3-Clause (code and graphics as published in that "
    "repository).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "kong", "Kong", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=pause|Hold Sel+Up=menu"
};
