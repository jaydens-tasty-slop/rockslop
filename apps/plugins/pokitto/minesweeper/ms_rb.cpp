/* Rockbox game description for MineSweeper. */
#include "Pokitto.h"

/* SELECT is A (uncover a tile, confirm); SELECT+DOWN is B (flag a tile). */
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
    "Uncover every tile that hides no bomb. A number counts the bombs "
    "in the eight neighbouring tiles.", "",
    "Directions: move the cursor; choose the difficulty on the title screen.", "",
    "Select: uncover the tile.", "",
    "Select+Down: flag the tile (and cycle flag, question mark, none).", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "MineSweeper", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/MineSweeper_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_minesweeper/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_minesweeper", "MineSweeper", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=dig  Sel+Down=flag|Hold Sel+Up=menu"
};
