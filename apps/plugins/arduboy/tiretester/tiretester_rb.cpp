/* Rockbox game description for Tire Tester. SELECT = B , SELECT + DOWN = A. */
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
    "Set up a tire launcher, rev the engine and send the tire as far as it will go.", "",
    "Up / Down: change the setting on each setup screen.", "",
    "Select (B): next screen; at the start line hold to rev, release to launch; after a run, go again.", "",
    "Select + Down (A): back one screen; after a run, back to the title.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tire Tester", "",
    "jaychugstein, github.com/jaychugstein/Tire-Tester", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tiretester", "Tire Tester", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, "tiretester/fxdata.bin", 0,
    "Sel=next  Sel+Down=back|Sel+Up=menu"
};
