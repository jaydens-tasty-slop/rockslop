/* Rockbox game description for FlappyBall. Select = B (jump); Up/Down also jump, Left toggles sound. */
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
    "Bounce the ball between the pipes.", "",
    "Select: flap (B).", "",
    "Left: sound on/off.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "FlappyBall",
    "",
    "Chris Martinez (2014); Scott Allen (2016).",
    "",
    "https://github.com/ArduboyCollection/FlappyBall @ 22253a6",
    "",
    "License: BSD 3-Clause. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "flappyball", "FlappyBall", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=flap  Left=sound|Hold Sel+Up=menu"
};
