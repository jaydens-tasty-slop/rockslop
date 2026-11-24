/* Rockbox game description for JumpBoy 2. Select = B (fire/start); A = Select+Down (fire); directions move. */
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
    "A coin-collecting jumper with enemies: five coins make an enemy "
    "appear, ten make a key appear. Grab the key.", "",
    "Left/Right: move. Up: jump.", "",
    "Select: start / fire (B).", "",
    "Select + Down: fire (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "JumpBoy 2",
    "",
    "takaiwa / t-iwasaki (2016, 2020).",
    "",
    "https://github.com/t-iwasaki/arduboy-JumpBoy2 @ 09a10fc",
    "",
    "License: MIT. Music and effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "jumpboy2", "JumpBoy 2", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=fire|Hold Sel+Up=menu"
};
