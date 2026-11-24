/* Rockbox game description for CosmicPods. Select = B (fire); A or B fire. */
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
    "As a cosmic squid, shoot the octopuses.", "",
    "Directions: move the squid.", "",
    "Select: fire.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "CosmicPods",
    "",
    "cubic9com (2017).",
    "",
    "https://github.com/ArduboyCollection/CosmicPods @ 0923919",
    "",
    "License: BSD 3-Clause. Effects through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "cosmicpods", "CosmicPods", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire|Hold Sel+Up=menu"
};
