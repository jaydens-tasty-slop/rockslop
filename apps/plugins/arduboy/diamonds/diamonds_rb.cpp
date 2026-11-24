/* Rockbox game description for Diamonds. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "A breakout game.", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Diamonds", "",
    "Redbug / kyuranGames, graphics by Doug Cannon.", "",
    "https://github.com/ArduboyCollection/diamonds", "",
    "License: MIT.", "",
    "Removed: the title tune (film theme, no licence).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "diamonds", "Diamonds", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
