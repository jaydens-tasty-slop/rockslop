/* Rockbox game description for ArduGo. Select = A (place stone); Select+Down = B (restart). */
#include <Arduino.h>

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
    "A 9x9 Go board against a simple computer opponent: place stones, "
    "capture prisoners.", "",
    "Directions: move the cursor. Select: place a black stone (A). "
    "Sel+Down: restart (B).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "ArduGo",
    "",
    "Modus Create, 2018 (Don Anderson).",
    "",
    "https://github.com/ModusCreateOrg/ardugo @ 2343b90",
    "",
    "License: MIT. Beeps through the Arduboy v1 tunes shim.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ardugo", "ArduGo", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=cursor  Sel=place|Sel+Dn=restart|Hold Sel+Up=menu"
};
