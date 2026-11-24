/* Rockbox game description for Blob Attack. B rotates right, A left, so
 * SELECT = B, SELECT + DOWN = A. */
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
    "A falling-blob puzzle: line up blobs of the same colour to clear "
    "them before the stack reaches the top.", "",
    "Left/Right: move. Down: drop faster. Up: pause.", "",
    "Select: rotate right (B); confirms in menus.", "",
    "Select + Down: rotate left (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Blob Attack", "",
    "TEAM a.r.g. (2016).", "",
    "https://github.com/Team-ARG-Museum/ID-28-Blob-Attack", "",
    "Code and art: MIT licence, (c) TEAM a.r.g.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "blobattack", "Blob Attack", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=rot R  Sel+Down=rot L|Up=pause  Hold Sel+Up=menu"
};
