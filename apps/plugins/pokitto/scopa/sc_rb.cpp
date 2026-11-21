/* Rockbox game description for Scopa. */
#include "Pokitto.h"

/* SELECT is A (confirm, play the card); SELECT+DOWN is B (back, cancel). */
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
    "Scopa, the Italian fishing card game: play a card from your hand to "
    "take table cards of the same value or that add up to it. Sweep the "
    "table for a bonus point (a scopa). Play against the computer or pass "
    "the player between two people.", "",
    "Directions: move the cursor.", "",
    "Select: confirm, play the card.", "",
    "Select+Down: back or cancel.", "",
    "Hold Select+Up for 0.5 s: pause menu.", "",
    "The images load into the music buffer, so the music player stops while it runs.",
    NULL
};

static const char *const credits[] = {
    "Scopa", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/Scopa_V1_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Font C64: MikroElektronika.", "",
    "Music and sounds are on the card (rocks/games/pokitto_scopa/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_scopa", "Scopa", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
