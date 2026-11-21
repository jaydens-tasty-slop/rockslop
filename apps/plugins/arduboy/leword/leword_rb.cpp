/* Rockbox game description for LeWord. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "A five-letter word guessing game (Wordle clone).", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "LeWord", "",
    "Press Play On Tape.", "",
    "https://github.com/Press-Play-On-Tape/LeWord", "",
    "License: BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "leword", "LeWord", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, "leword/fxdata.bin", 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
