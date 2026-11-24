/* Rockbox game description for Dark Trails (ButtonsTrail). */
#include "Pokitto.h"

/* SELECT is A (confirm, step); SELECT+DOWN is B (leave the puzzle, back). */
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
    "A path puzzle: lay a trail over the board by following the rules "
    "of each level.", "",
    "Directions: move.", "",
    "Select: confirm, choose a level.", "",
    "Select+Down: leave the puzzle, back.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Dark Trails (ButtonsTrail)", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/ButtonsTrail_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Font 5x7: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_buttonstrail/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_buttonstrail", "Dark Trails", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=OK  Sel+Down=back|Hold Sel+Up=menu"
};
