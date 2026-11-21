/* Rockbox game description for Scrambler. */
#include "Pokitto.h"

/* The ship flies with the directions. A (laser) is a SELECT tap and B (bomb)
 * a SELECT hold, so both work while steering. */
static const struct pk_rule rules[] = {
    {PK_P_UP,    PK_B_UP},
    {PK_P_DOWN,  PK_B_DOWN},
    {PK_P_LEFT,  PK_B_LEFT},
    {PK_P_RIGHT, PK_B_RIGHT},
    {0, 0}
};

static const struct pk_keys keys = {rules, PK_B_A, PK_B_B, 300};
static const struct pk_keys *current_keys() { return &keys; }

static const char *const help[] = {
    "Fly the ship over the scrolling land, shoot the rockets and "
    "tanks, bomb the ground targets and pick up fuel.", "",
    "Directions: steer the ship.", "",
    "Select (tap): fire the laser.", "",
    "Select (hold a third of a second): drop a bomb.", "",
    "Menus: a Select tap confirms, a Select hold goes back.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Scrambler", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/Scrambler_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Font 5x7: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_scrambler/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_scrambler", "Scrambler", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel tap=laser|Sel hold=bomb|Hold Sel+Up=menu"
};
