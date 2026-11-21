/* Rockbox game description for Space Fighter. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT, AB_B_B},        /* fire (held), also starts from the title */
    {AB_P_UP,     AB_B_UP},
    {AB_P_DOWN,   AB_B_DOWN},
    {AB_P_LEFT,   AB_B_LEFT},
    {AB_P_RIGHT,  AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A vertical space shooter in the style of R-Type.", "",
    "Directions: move the ship.", "",
    "Select: fire (hold for a stream of bullets); starts the game on the "
    "title screen. The game's own pause (A) is not needed: use the pause "
    "menu.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Space Fighter", "",
    "Maicon Hieronymus, 2015.", "",
    "https://github.com/ArduboyCollection/SpaceFighter", "",
    "License: GPL-3.0 or later.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "spacefighter", "Space Fighter", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=fire|Hold Sel+Up=menu"
};
