/* Rockbox game description for Pyoro. Default mapping: SELECT = A (tongue /
 * start), directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "Pyoro the bird catches the falling beans with his tongue before they "
    "eat the floor.", "",
    "Left/Right: walk.", "",
    "Select: stick out the tongue; start the game.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pyoro", "",
    "A clone of the Pyoro game, by Thesola10.", "",
    "https://github.com/Thesola10/Pyoro", "",
    "ArdBitmap by Ignacio Vina (Apache-2.0).", "",
    "License: GPL-3.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "pyoro", "Pyoro", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=tongue  L/R=walk|Hold Sel+Up=menu"
};
