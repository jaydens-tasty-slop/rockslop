/* Rockbox game description for Morse Code Trainer. The OBONO games confirm with B and use
 * A for cancel / menu, so: SELECT = B, SELECT + DOWN = A. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Decode Morse code to Alphabet or Japanese letters.", "",
    "Up: backspace, Down: new line.", "",
    "Select: the B button: the straight key (hold = dash, tap = dot), also confirm.", "",
    "Left / Right: the iambic paddle (dot / dash); the two cannot be squeezed on this player.", "",
    "Select + Down: the A button (cancel, menu, sound on/off in menus).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Morse Code Trainer", "",
    "OBONO (2016-2026).", "",
    "https://github.com/obono/ArduboyWorks", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "morse", "Morse Code Trainer", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=key(B)  Sel+Dn=menu(A)|L/R=paddle  Up=del|Hold Sel+Up=menu"
};
