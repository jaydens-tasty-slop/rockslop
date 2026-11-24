/* Rockbox game description for Rogue. */
#include "Pokitto.h"

/* SELECT is A (confirm, attack, go on); SELECT+DOWN is B (inventory, save and quit). */
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
    "A small roguelike: explore 26 floors of dungeon, fight monsters, "
    "collect gold, food and gear, and find the stairs down.", "",
    "Directions: move; walking into a monster attacks it.", "",
    "Select: confirm, pick up, go on.", "",
    "Select+Down: inventory, save and quit (the game's menus say B).", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Rogue", "",
    "Original game by Press Play On Tape.", "",
    "https://github.com/Press-Play-On-Tape/Rogue_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Font 5x7: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_rogue", "Rogue", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
