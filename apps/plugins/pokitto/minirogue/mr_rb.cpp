/* Rockbox game description for Mini Rogue. */
#include "Pokitto.h"

/* SELECT is A (confirm, roll, act); SELECT+DOWN is B (leave the merchant);
 * SELECT+LEFT is C (clear the scores on the game over screen). */
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
    "A solo dungeon crawl on a board of rooms: fight monsters, "
    "dodge traps, trade with merchants and descend through the areas.", "",
    "Directions: move the cursor, choose items and skill level.", "",
    "Select: confirm, roll the dice, act.", "",
    "Select+Down: leave the merchant and go on to the next room.", "",
    "Select+Left: on the game over screen, clear the high score.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Mini Rogue", "",
    "Pokitto port by Press Play On Tape (Filmote). Based on the board game "
    "Mini Rogue by Gabriel Gendron.", "",
    "https://github.com/Press-Play-On-Tape/MiniRogue_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_minirogue/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_minirogue", "Mini Rogue", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=OK  Sel+Down=leave|Hold Sel+Up=menu"
};
