/* Rockbox game description for MicroCity. Select = B (build, confirm); Sel+Down = A (toolbar, back). D-pad moves the cursor. */
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
    "City simulation inspired by SimCity: build roads, power and zones, "
    "balance the budget.", "",
    "D-pad: move cursor. Select (B): build or confirm. Select+Down (A): "
    "open the toolbar or go back. Save and load a city from the toolbar "
    "menu.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "MicroCity",
    "",
    "James Howard (jhhoward).",
    "",
    "https://github.com/jhhoward/MicroCity @ ae265f1",
    "",
    "License: GPL 3.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "microcityab", "MicroCity", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "D-pad=move  Sel=build|SelDn=toolbar|Hold Sel+Up=menu"
};
