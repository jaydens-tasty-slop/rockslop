/* Rockbox game description for Lights Out. */
#include "Pokitto.h"

/* SELECT is A (toggle the lights, confirm); SELECT+DOWN is B (back on the
 * title screen). C (switch between the board and the menu) is SELECT+LEFT. */
static const struct pk_rule rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_B},
    {PK_P_SELECT | PK_P_LEFT, PK_B_C},
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
    "Turn off all the lights. Switching a light also switches its "
    "neighbours above, below, left and right.", "",
    "Directions: move the cursor.", "",
    "Select: switch the light under the cursor.", "",
    "Select+Left: swap between the board and the level menu.", "",
    "Select+Down: back (title screen).", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Lights Out", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/LightsOut_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Font 5x7: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_lightsout/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_lightsout", "Lights Out", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=switch|Sel+Left=levels|Hold Sel+Up=menu"
};
