/* Rockbox game description for Ardu Buggy. Select = B (jump), Select+Down = A (fire); Up/Down switch sound on/off. */
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
    "Drive the moon buggy: jump over craters and shoot the aliens and "
    "rocks.", "",
    "Left/Right: speed up and slow down.", "",
    "Select: jump (B); starts the game.", "",
    "Select + Down: fire (A).", "",
    "Up / Down: sound on / off.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ardu Buggy",
    "",
    "Ardu Buggy, a Moon Patrol style game (see README upstream).",
    "",
    "https://github.com/ArduboyCollection/ArduBuggy @ 65ddafa",
    "",
    "License: Unlicense (public domain). Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardubuggy", "Ardu Buggy", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=fire|Hold Sel+Up=menu"
};
