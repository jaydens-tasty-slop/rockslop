/* Rockbox game description for PiCross. */
#include "Pokitto.h"

/* SELECT alone is tap/hold: a tap is A (fill a cell, confirm), holding it
 * is B (mark a cell, go back). SELECT held with a direction drags A, which
 * paints the cells passed over; up is not possible there (SELECT+UP is the
 * menu gesture). Two quick taps (patched in PlayGameState_Update.cpp) open
 * the game's own menu (C). */
static const struct pk_rule rules[] = {
    {PK_P_SELECT, PK_B_A},
    {PK_P_UP,     PK_B_UP},
    {PK_P_DOWN,   PK_B_DOWN},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {0, 0}
};

static const struct pk_keys keys = {rules, PK_B_A, PK_B_B, 300};
static const struct pk_keys *current_keys() { return &keys; }

static const char *const help[] = {
    "Fill the grid so that each row and column has runs of filled cells "
    "matching its clues. Marked cells (crosses) are cells you know are "
    "empty.", "",
    "Directions: move the cursor.", "",
    "Select (tap): fill or clear the cell.", "",
    "Select (hold): mark or unmark the cell; also goes back in menus.", "",
    "Select held with Left, Right or Down: drag to paint the cells you "
    "pass over (start at the top or left end of a line; dragging up is "
    "not possible).", "",
    "Select twice quickly: the game's menu (hint, leave).", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "PiCross", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/PiCross_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Removed: the four tunes (no author or licence upstream), so there is no music.", "",
    "Font 5x7: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_picross", "PiCross", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1,
    "Sel=fill  Hold Sel=mark|Sel+Dir=drag  2xSel=menu|Hold Sel+Up=menu"
};
