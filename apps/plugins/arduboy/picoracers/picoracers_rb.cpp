/* Rockbox game description for Pico Racers. Select = A (accelerate), Select+Left = B (menu confirm); Up/Down steer. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LEFT, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A side-view racer: hold the throttle, weave between the other cars "
    "and finish the course.", "",
    "Up / Down: steer.", "",
    "Select: accelerate (A).", "",
    "Select + Left: confirm in menus (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pico Racers",
    "",
    "emuty, PixelArtM, JO3RI (TEAM a.r.g., 2016).",
    "",
    "https://github.com/Team-ARG-Museum/ID-43-Pico-Racers @ ff088cd",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g. Sound through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "picoracers", "Pico Racers", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "U/D=steer  Sel=gas|Hold Sel+Up=menu"
};
