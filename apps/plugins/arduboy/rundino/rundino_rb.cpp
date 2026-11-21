/* Rockbox game description for RUND.INO. Select = A (start, jump, restart). */
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
    "The offline dinosaur runner: jump over the cacti and run as far as "
    "you can.", "",
    "Select: start, jump, and restart after a tumble.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "RUND.INO",
    "",
    "Flaki (arduboy-rund-ino).",
    "",
    "https://github.com/ArduboyCollection/arduboy-rund-ino @ ff8615f "
    "(original: github.com/flaki/arduboy-rund-ino)",
    "",
    "License: ISC (declared in the author's package.json). Sounds "
    "through the Arduboy v1 tunes API.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "rundino", "RUND.INO", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=jump|Hold Sel+Up=menu"
};
