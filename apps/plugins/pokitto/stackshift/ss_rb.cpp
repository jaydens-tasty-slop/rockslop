/* Rockbox game description for StackShift. */
#include "Pokitto.h"

/* SELECT is A (rotate the column down); SELECT+DOWN is B (rotate up). UP drops
 * the piece. The game's own pause screen (C) is not mapped: use the Rockbox
 * pause menu. */
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
    "A falling-block puzzle: shift and rotate the falling column so that "
    "matching symbols line up and clear.", "",
    "Left, Right: move the piece.", "",
    "Up: drop the piece.", "",
    "Select: rotate the column down.", "",
    "Select+Down: rotate the column up.", "",
    "Hold Select+Up for 0.5 s: pause menu.", "",
    "The images load into the music buffer, so the music player stops while "
    "it runs.",
    NULL
};

static const char *const credits[] = {
    "StackShift", "",
    "Original game by Press Play On Tape, based on an idea by @Batske "
    "(NewBlocksOnTheKid).", "",
    "https://github.com/Press-Play-On-Tape/StackShift_Pokitto", "",
    "License: GPL-3.0.", "",
    "Font C64: MikroElektronika.", "",
    "Music and sounds are on the card (rocks/games/pokitto_stackshift/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_stackshift", "StackShift", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1,
    "Sel=rot  Sel+Down=back|Up=drop  Hold Sel+Up=menu"
};
