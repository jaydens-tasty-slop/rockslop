/* Rockbox game description for Kong II. Default key mapping: SELECT = A
 * (jump), SELECT + DOWN = B (pause, hard mode on the title screen). */
#include <Arduino.h>

static const char *const help[] = {
    "Junior must reach the keys across the vines and bring them to the "
    "locks. Dodge the birds and sparks, and the clappers.", "",
    "Left/Right: run. Up/Down: climb vines.", "",
    "Select: jump (also starts an easy game on the title screen).", "",
    "Select + Down: pause (starts a hard game on the title screen).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Kong II", "",
    "Press Play On Tape, Filmote and contributors (2019).", "",
    "https://github.com/Press-Play-On-Tape/Kong-II", "",
    "License: BSD-3-Clause (code and graphics as published in that "
    "repository).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "kong2", "Kong II", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=pause|Hold Sel+Up=menu"
};
