/* Rockbox game description for LeWord. */
#include "Pokitto.h"

/* SELECT is A (pick the letter under the cursor); SELECT+DOWN is B (delete
 * a letter, hold to give up the puzzle). C (pause) is the pause menu. */
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
    "Guess the hidden word in six tries. After each guess the letters "
    "show whether they are in the word and in the right place.", "",
    "Directions: move over the keyboard.", "",
    "Select: type the letter under the cursor.", "",
    "Select+Down: delete a letter; hold it to quit the puzzle.", "",
    "Hold Select+Up for 0.5 s: pause menu.", "",
    "The images load into the music buffer, so the music player stops while "
    "it runs.",
    NULL
};

static const char *const credits[] = {
    "LeWord", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/LeWord_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Removed: the three tunes (no author or licence upstream), so there is no music.", "",
    "Word lists and sound effects are on the card (rocks/games/pokitto_leword/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_leword", "LeWord", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=letter  Sel+Down=delete|Hold Sel+Up=menu"
};
