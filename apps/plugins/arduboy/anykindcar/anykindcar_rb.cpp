/* Rockbox game description for Any Kind of Car and Any Kind of Truck. Select = B (randomize); Select+Down = A (title toggle). Big/baby chords unavailable. */
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
    "Make randomly dimensioned vehicles and move them around.", "",
    "Left/Right: drive. Up: jump. Down: crouch.", "",
    "Select: new vehicle. Sel+Down: toggle title.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Any Kind of Car and Any Kind of Truck",
    "",
    "Tommy Marshall (rocktronica), 2024.",
    "",
    "https://github.com/rocktronica/any-kind-of-car-and-any-kind-of- "
    "truck @ 911c662",
    "",
    "License: MIT. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "anykindcar", "Any Kind of Car and Any Kind of Truck", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=new car|Sel+Dn=title|Hold Sel+Up=menu"
};
