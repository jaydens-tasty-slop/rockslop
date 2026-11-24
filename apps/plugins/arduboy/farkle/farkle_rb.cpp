/* Rockbox game description for Farkle. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "The dice game Farkle against the computer.", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Farkle", "",
    "Press Play On Tape.", "",
    "https://github.com/Press-Play-On-Tape/Farkle", "",
    "License: Apache-2.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "farkle", "Farkle", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
