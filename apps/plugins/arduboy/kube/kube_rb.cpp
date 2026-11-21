/* Rockbox game description for KUBE. SELECT = A (jump). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT, AB_B_A},
    {AB_P_UP,     AB_B_UP},
    {AB_P_DOWN,   AB_B_DOWN},
    {AB_P_LEFT,   AB_B_LEFT},
    {AB_P_RIGHT,  AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Guide the cube through the isometric rooms, avoid spikes and bugs.", "",
    "Directions: move.", "",
    "Select: jump (A button).", "",
    "Greyscale is dithered to the 1-bit screen.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "KUBE", "",
    "Onebit Productions (Matthew Bryan), github.com/TropicAnthropic/KUBE", "",
    "ArduboyG replaced by a YP3 dither shim.", "",
    "License: GPL-3.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "kube", "KUBE", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=jump|Sel+Up=menu"
};
