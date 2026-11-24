/* Rockbox game description for German Whist. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "The two-player trick-taking card game German Whist.", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "German Whist", "",
    "Press Play On Tape.", "",
    "https://github.com/Press-Play-On-Tape/GermanWhist", "",
    "License: BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "germanwhist", "German Whist", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
