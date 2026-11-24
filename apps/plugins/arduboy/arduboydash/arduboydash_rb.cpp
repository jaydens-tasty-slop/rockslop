/* Rockbox game description for Ardu Dash. Select = A (start); Up or Select jumps; Down pauses. */
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
    "A Geometry Dash style runner: jump over spikes and blocks.", "",
    "Select, A, B or Up: jump. Down: pause.", "",
    "Up+Down on the title screen resets the best score.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ardu Dash",
    "",
    "Sergei Petrov (SuperSolik), 2023.",
    "",
    "https://github.com/SuperSolik/arduboy-games @ 523b23a (ardu_dash)",
    "",
    "License: MIT. No sound upstream.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arduboydash", "Ardu Dash", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel/Up=jump  Down=pause|Hold Sel+Up=menu"
};
