/* Rockbox game description for Ardusweeper. Select = A (open a tile), Select+Down = B (flag). */
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
    "Minesweeper on a 15 x 9 grid with three difficulties and saved "
    "best times.", "",
    "Directions: move the cursor.", "",
    "Select: open the tile (A).", "",
    "Select + Down: flag / unflag (B); on an open tile in fast mode it "
    "opens the neighbours.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ardusweeper",
    "",
    "jbellue (2018).",
    "",
    "https://github.com/ArduboyCollection/minesweeper @ c228957",
    "",
    "License: GPL 3. Sounds through the v1 tunes shim over "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardusweeper", "Ardusweeper", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=open  Sel+Down=flag|Hold Sel+Up=menu"
};
