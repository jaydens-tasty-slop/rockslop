/* Rockbox game description for Cannon Father. SELECT = B (go), SELECT + DOWN
 * = A (glide). */
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
    "Fire Father out of a cannon, bounce off everything and fly as far as you can.", "",
    "Up / Left: raise the cannon.  Down / Right: lower it.", "",
    "Select: start, set the aim, fire (hit the top of the power bar for a perfect shot).", "",
    "Hold Select + Down in the air to glide.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Cannon Father", "",
    "jaychugstein, github.com/jaychugstein/Cannon-Father", "",
    "Music: Ode to Joy (Beethoven, public domain).", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "cannonfather", "Cannon Father", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, "cannonfather/fxdata.bin", 0,
    "Sel=go  Sel+Down=glide|Sel+Up=menu"
};
