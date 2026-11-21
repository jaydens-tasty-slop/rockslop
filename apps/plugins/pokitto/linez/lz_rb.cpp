/* Rockbox game description for Linez. */
#include "Pokitto.h"

extern bool pk_linez_playing(void);

/* While playing SELECT rotates (A) and SELECT+DOWN stores the block (B).
 * On the title and game over screens SELECT is C (start). */
static const struct pk_rule play_rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_B},
    {PK_P_SELECT,             PK_B_A},
    {PK_P_UP,                 PK_B_UP},
    {PK_P_DOWN,               PK_B_DOWN},
    {PK_P_LEFT,               PK_B_LEFT},
    {PK_P_RIGHT,              PK_B_RIGHT},
    {0, 0}
};
static const struct pk_rule menu_rules[] = {
    {PK_P_SELECT, PK_B_C},
    {PK_P_UP,     PK_B_UP},
    {PK_P_DOWN,   PK_B_DOWN},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {0, 0}
};
static const struct pk_keys play_keys = {play_rules, 0, 0, 0};
static const struct pk_keys menu_keys = {menu_rules, 0, 0, 0};

static const struct pk_keys *current_keys()
{
    return pk_linez_playing() ? &play_keys : &menu_keys;
}

static const char *const help[] = {
    "Falling blocks: complete rows to clear them.", "",
    "Left/Right: move the block. Down: drop it faster.", "",
    "Select: rotate the block.", "",
    "Select+Down: store the block and swap it with the stored one.", "",
    "Title and game over screens: Select starts.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Linez", "",
    "Original game by sbmrgd, after a tutorial by Javier Lopez Lopez.", "",
    "https://github.com/sbmrgd/linez", "",
    "License: BSD-3-Clause.", "",
    "Font 3x5: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_linez", "Linez", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=rotate  Sel+Down=store|Hold Sel+Up=menu"
};
