/* Rockbox game description for Shooter. Select = B (fire), Select+Down = A (confirm/pause). */
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
    "Drive the tank, aim the cannon and shoot the aliens while avoiding "
    "potholes.", "",
    "Left/Right/Up/Down: move and aim.", "",
    "Select: fire (B).", "",
    "Select + Down: confirm / pause (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Shooter",
    "",
    "Sandor Kovacs (2021).",
    "",
    "https://github.com/shantheone/shooter @ cb40c26",
    "",
    "License: MIT. Effects play through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "shooter", "Shooter", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=fire|Hold Sel+Up=menu"
};
