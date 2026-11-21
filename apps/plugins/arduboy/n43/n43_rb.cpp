/* Rockbox game description for Nineteen43. */
#include <Arduino.h>

/* A (fire) is pressed far more often than B (barrel roll), and moving down
 * while firing must not roll, so: SELECT tapped = A (fires on release),
 * SELECT held 0.4 s = B. The first rule swallows the menu chord so it does
 * not also press a button when Up is held first. */
static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_UP,   0},
    {AB_P_SELECT | AB_P_LONG, AB_B_B},
    {AB_P_TAP,                AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Vertical shoot-'em-up over the Pacific: destroy the enemy Zeroes and "
    "Bombers. Watch your fuel, health and ammunition and refill them by "
    "flying over the power-ups.", "",
    "Directions: fly.", "",
    "Tap Select: fire (it fires on release).", "",
    "Hold Select (0.4 s): barrel roll, to dodge bullets and enemies.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Nineteen43", "",
    "Simon Holmes (Filmote), 2017.", "",
    "https://github.com/filmote/Nineteen43", "",
    "License: MIT. FixedPoints library: Pharap, Apache-2.0.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "n43", "Nineteen43", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 400, 0, 0, NULL, 0,
    "Tap Sel=fire  Hold Sel=roll|Hold Sel+Up=menu"
};
