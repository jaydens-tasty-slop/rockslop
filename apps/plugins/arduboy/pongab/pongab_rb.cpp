/* Rockbox game description for Pong (Arduboy). Select = A (start, player 2 down), Select+Right = B (player 2 up). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_RIGHT, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Classic Pong against the computer or a second player on the same "
    "keys.", "",
    "Up/Down: move your paddle; Left pauses.", "",
    "Left/Right on the setup screen: one or two players.", "",
    "Select: start (A); in two-player games player 2 moves down with "
    "Select and up with Select + Right (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pong (Arduboy)",
    "",
    "Taylor Hansen (2017).",
    "",
    "https://github.com/ArduboyCollection/Pong-Arduboy @ 840b42a",
    "",
    "License: MIT. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "pongab", "Pong (Arduboy)", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Right=B|Hold Sel+Up=menu"
};
