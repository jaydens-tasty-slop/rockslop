/* Rockbox game description for Block Stacker. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "A falling block stacking game.", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Block Stacker", "",
    "David Martinez (DMG), 2015.", "",
    "https://github.com/ArduboyCollection/BlockStacker", "",
    "License: LGPL-2.1 or later (game code); Arduboy lib BSD.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "blockstacker", "Block Stacker", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
