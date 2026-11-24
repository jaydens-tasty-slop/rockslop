/* Rockbox game description for Waternet. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "A pipe connecting puzzle game.", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Waternet", "",
    "Willems Davy (joyrider3774).", "",
    "https://github.com/joyrider3774/waternet_arduboy", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "waternetab", "Waternet (Arduboy)", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
