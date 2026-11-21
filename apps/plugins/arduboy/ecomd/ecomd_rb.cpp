/* Rockbox game description for Epic Crates of Mass Destruction. Select = B (jump), Select+Down = A (fire); Up = pause. */
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
    "Collect crates, pick up weapons and survive the waves of enemies "
    "in the arena.", "",
    "Left/Right: move.", "",
    "Select: jump (B).", "",
    "Select + Down: fire (A).", "",
    "Up: pause.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Epic Crates of Mass Destruction",
    "",
    "TEAM a.r.g.: DRAGULA96, JO3RI, Justin Cyr (2015).",
    "",
    "https://github.com/Team-ARG-Museum/ID-33-ECOMD @ c2dac94",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g. Effects through "
    "ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ecomd", "Epic Crates of Mass Destruction", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=move  Sel=jump|Hold Sel+Up=menu"
};
