/* Rockbox game description for Crates 3D. Select = B (push), Select+Down = A (rotate view; hold to quit the level), Select+Left = A+B (save). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LEFT, AB_B_A | AB_B_B},
    {AB_P_SELECT | AB_P_DOWN, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A 3D crate-pushing puzzle after the TI-83 game: guide the black "
    "block to the other black block.", "",
    "Directions: move.", "",
    "Select + a direction: push a crate (B).", "",
    "Select + Down: rotate the view (A); hold it to quit or restart the "
    "level.", "",
    "Select + Left: A+B, saves progress.", "",
    "On the title screen Left/Right choose the level, Select starts.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Crates 3D",
    "",
    "Ion version by Badja; Arduboy port by Brian Smith (fuopy), 2016.",
    "",
    "https://github.com/ArduboyCollection/Crates3D @ 7573547",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "crates3d", "Crates 3D", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel+dir=push|Sel+Down=rotate|Hold Sel+Up=menu"
};
