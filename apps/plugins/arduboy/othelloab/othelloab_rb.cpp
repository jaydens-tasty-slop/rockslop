/* Rockbox game description for Pocket Othello. Select = A (place a disc, confirm); Sel+Down = B (pause, back). D-pad moves the cursor. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Othello against the computer: pick a side, then flank the "
    "opponent's discs to flip them.", "",
    "D-pad: move cursor. Select (A): place a disc. Select+Down (B): "
    "pause, and from the pause screen quit to the side menu.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pocket Othello",
    "",
    "Wang Renxin, 2017.",
    "",
    "https://github.com/ArduboyCollection/pocket_othello @ 5a03e05",
    "",
    "License: CC BY-SA 4.0 (as stated by the author). No sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "othelloab", "Pocket Othello", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "D-pad=cursor  Sel=place|SelDn=pause|Hold Sel+Up=menu"
};
