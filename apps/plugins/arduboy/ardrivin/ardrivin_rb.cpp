/* Rockbox game description for Ard-Drivin. */
#include <Arduino.h>

/* A = throttle (held), B = gear (pressed). SELECT = throttle,
 * SELECT + DOWN = gear. */
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
    "Outrun-style racing against the clock.", "",
    "Left/Right: steer. Up/Down: nudge the car on the road.", "",
    "Hold Select: accelerate (release to brake).", "",
    "Select + Down: change gear (low/high). Use high gear once the "
    "speed is up.", "",
    "Hold Select + Up for 1 s: pause menu (Select + Up is also gas and a "
    "nudge, hence the longer hold). The title and game over screens start "
    "the game with Select.",
    NULL
};

static const char *const credits[] = {
    "Ard-Drivin", "",
    "(c) 2018 Remi Veilleux (code) and Louis-Philippe Guilbert (graphics).", "",
    "https://github.com/rveilleux/ard-drivin", "",
    "License: MIT.",
    NULL
};

/* The game fakes grey by flickering: the layer blends successive frames. */
extern "C" const struct ab_game ab_game = {
    "ardrivin", "Ard-Drivin", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 1000, 0, 0, 0, NULL, 1,
    "Sel=gas  Sel+Down=gear|Hold Sel+Up=menu"
};
