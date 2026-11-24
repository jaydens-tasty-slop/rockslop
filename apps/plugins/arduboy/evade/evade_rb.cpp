/* Rockbox game description for Evade. B is the rapid weapon, A the heavy one, both held: SELECT = B, SELECT + DOWN = A. */
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
    "Fly your ship through the asteroid fields and fight the waves of "
    "enemies.", "",
    "Directions: fly the ship.", "",
    "Select: rapid fire (B); starts the game.", "",
    "Select + Down: heavy shot (A).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Evade",
    "",
    "Modus Create (2016).",
    "",
    "https://github.com/ModusCreateOrg/evade-arduboy-game",
    "",
    "License: MIT. Music and effects are played through the layer's "
    "Playtune and tone engines.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "evade", "Evade", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=rapid  Sel+Down=heavy|Hold Sel+Up=menu"
};
