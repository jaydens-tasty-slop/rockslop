/* Rockbox game description for Hello, Commander. Default key mapping:
 * SELECT = A (confirm, fire), SELECT + DOWN = B (cancel, end turn). */
#include <Arduino.h>

static const char *const help[] = {
    "A turn-based strategy game: lead your squad across the map, spend "
    "action points to move and shoot, and complete the Commander's "
    "missions. Soldiers level up; there are no saves.", "",
    "Directions: move the cursor or soldier.", "",
    "Select: the A button (confirm, act).", "",
    "Select + Down: the B button (cancel, back, end turn).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Hello, Commander", "",
    "Felipe Manga (2017).", "",
    "https://github.com/felipemanga/HelloCommander", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "hellocmd", "Hello, Commander", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A (OK)  Sel+Down=B|Hold Sel+Up=menu"
};
