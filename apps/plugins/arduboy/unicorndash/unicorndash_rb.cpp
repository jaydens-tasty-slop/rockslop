/* Rockbox game description for Unicorn Dash. Select = B (jump), Select+Down = A (pause/credits). */
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
    "A tiny endless runner: dash as a unicorn, jump over the obstacles "
    "and go as far as you can.", "",
    "Left/Right: nudge the unicorn.", "",
    "Select: jump (B).", "",
    "Select + Down: pause (A); on the title, credits.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Unicorn Dash",
    "",
    "Kirill Korolkov (2018).",
    "",
    "https://github.com/KirillKorolkov/unicorn-dash @ c134b4c",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "unicorndash", "Unicorn Dash", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=move  Sel=jump|Hold Sel+Up=menu"
};
