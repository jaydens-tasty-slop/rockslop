/* Rockbox game description for Ardulem. */
#include <Arduino.h>

/* In the original B (action) is the common key and A + directions moves the
 * tool selection. SELECT tapped = B; SELECT held = A (then directions work
 * as A + direction). */
static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LONG, AB_B_A},
    {AB_P_TAP,                AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Guide the lemmings to the exit. Left/Right/Up/Down move the cursor.", "",
    "Tap Select: give the selected order to the lemming under the cursor "
    "(the B button); also pause on the timer, abandon on the flag.", "",
    "Hold Select (0.3 s): the A button. Press it once to select the next "
    "tool, or keep holding and use the directions to move around the "
    "toolbar, set the drop speed or switch Speedy mode.", "",
    "Hold Select + Up for 1 s: pause menu (the 0.3 s hold is the A button "
    "plus Up, so the menu takes longer).",
    NULL
};

static const char *const credits[] = {
    "Ardulem", "",
    "(c) 2017 Alban Nanty.", "",
    "https://github.com/Lswbanban/Ardulem", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardulem", "Ardulem", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 1000, 300, 0, 0, NULL, 0,
    "Tap=B  Hold=A|Hold Sel+Up=menu"
};
