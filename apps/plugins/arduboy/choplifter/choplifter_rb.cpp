/* Rockbox game description for Choplifter. Default key mapping: SELECT = A
 * (fire, drop), SELECT + DOWN = B (turn the helicopter). */
#include <Arduino.h>

static const char *const help[] = {
    "Fly your helicopter into enemy territory, rescue the hostages and "
    "bring them home. Shoot tanks and jets; do not crash.", "",
    "Left/Right: fly. Up: climb (the helicopter sinks when you let go).", "",
    "Select: fire.", "",
    "Select + Down: turn the helicopter around (when hovering) / pick the "
    "hard level on the title screen.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Choplifter", "",
    "Filmote (2018).", "",
    "https://github.com/Press-Play-On-Tape/Choplifter", "",
    "License: BSD-3-Clause (code and graphics as published in that "
    "repository).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "choplifter", "Choplifter", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+Down=turn|Hold Sel+Up=menu"
};
