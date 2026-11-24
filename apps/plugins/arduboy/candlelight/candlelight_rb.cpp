/* Rockbox game description for Candlelight. Select = A (jump / start), Select+Down = B (dash). */
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
    "A dark platformer: carry your candle through the rooms.", "",
    "Left/Right: run; on the title they switch sound off/on.", "",
    "Select: jump / start (A).", "",
    "Select + Down: dash (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Candlelight",
    "",
    "Matthew Bryan (2024).",
    "",
    "https://github.com/TropicAnthropic/Candlelight @ 9a48b56",
    "",
    "License: GPL 3. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "candlelight", "Candlelight", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump  Sel+Down=dash|Hold Sel+Up=menu"
};
