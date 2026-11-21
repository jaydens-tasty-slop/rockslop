/* Rockbox game description for CastleBoy. Default key mapping: SELECT = A
 * (jump, confirm), SELECT + DOWN = B (sword), UP = throw knife, DOWN = duck. */
#include <Arduino.h>

static const char *const help[] = {
    "A side-scrolling castle adventure: fight through the castle with "
    "sword and knives, collect coins and beat the bosses.", "",
    "Left/Right: walk. Down: duck. Up: throw a knife.", "",
    "Select: jump (and confirm in menus).", "",
    "Select + Down: sword attack.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "CastleBoy", "",
    "dir3kt (2016).", "",
    "https://github.com/dir3kt/CastleBoy", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "castleboy", "CastleBoy", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=sword|Up=knife  Hold Sel+Up=menu"
};
