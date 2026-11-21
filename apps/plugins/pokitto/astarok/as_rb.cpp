/* Rockbox game description for Trials of Astarok. */
#include "Pokitto.h"

/* SELECT is A (jump, hold for height); SELECT+DOWN is B (open a chest, read
 * a sign). C (pause) is the pause menu. */
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
    "Run through the Trials of Astarok as far as you can: jump on "
    "enemies, collect coins and runes, and reach the doors.", "",
    "Left/Right: run.", "",
    "Select: jump (hold for a higher jump). Menus: confirm.", "",
    "Select+Down: open a chest, read a sign.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Trials of Astarok", "",
    "Original game by Press Play On Tape (Filmote and others).", "",
    "https://github.com/Press-Play-On-Tape/TrialsOfAstarok_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_astarok/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_astarok", "Trials of Astarok", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=jump  Sel+Down=open|Hold Sel+Up=menu"
};
