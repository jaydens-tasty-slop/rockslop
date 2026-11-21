/* Rockbox game description for Noughts and Crosses. */
#include "Pokitto.h"

/* SELECT is A (place a mark); SELECT+DOWN is B (hold it to restart after a game). */
static const struct pk_rule rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_B},
    {PK_P_SELECT,             PK_B_A},
    {PK_P_UP,                 PK_B_UP},
    {PK_P_DOWN,               PK_B_DOWN},
    {PK_P_LEFT,               PK_B_LEFT},
    {PK_P_RIGHT,              PK_B_RIGHT},
    {0, 0}
};

static const struct pk_keys keys = {rules, 0, 0, 0};
static const struct pk_keys *current_keys() { return &keys; }

static const char *const help[] = {
    "Noughts and crosses for two players taking turns on the same device: "
    "get three of your marks in a row.", "",
    "Directions: move the cursor.", "",
    "Select: place a mark.", "",
    "Select+Down (hold): start a new game once one has finished.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Noughts and Crosses", "",
    "Original game by Pharap (Pharap/SimpleNoughtsAndCrosses).", "",
    "https://github.com/Pharap/SimpleNoughtsAndCrosses", "",
    "License: Apache-2.0.",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_noughts", "Noughts and Crosses", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1,
    "Sel=mark  Sel+Down=restart|Hold Sel+Up=menu"
};
