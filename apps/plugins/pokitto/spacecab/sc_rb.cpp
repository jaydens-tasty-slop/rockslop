/* Rockbox game description for Space Cab. */
#include "Pokitto.h"
#include "src/SpaceCab.h"

extern Game game;

/* SELECT is A (thrust, hold it); DOWN is B (landing gear). On the high
 * score screen DOWN scrolls the letters instead. */
static const struct pk_rule rules[] = {
    {PK_P_UP,     PK_B_UP},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {PK_P_SELECT, PK_B_A},
    {PK_P_DOWN,   PK_B_B},
    {0, 0}
};
static const struct pk_rule score_rules[] = {
    {PK_P_UP,     PK_B_UP},
    {PK_P_DOWN,   PK_B_DOWN},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {PK_P_SELECT, PK_B_A},
    {0, 0}
};
static const struct pk_keys play_keys = {rules, 0, 0, 0};
static const struct pk_keys score_keys = {score_rules, 0, 0, 0};

static const struct pk_keys *current_keys()
{
    GameState s = game.getState();
    return s == GameState::HighScore || s == GameState::HighScore_Init ? &score_keys
                                                                       : &play_keys;
}

static const char *const help[] = {
    "Fly the space taxi: pick up passengers, take them to their "
    "platform before the fuel runs out, and land gently.", "",
    "Left/Right: steer.", "",
    "Select (hold): thrust.", "",
    "Down: lower or raise the landing gear (lower it before touching down).", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Space Cab", "",
    "Original game by Press Play On Tape (bloggingadeadhorse).", "",
    "https://github.com/Press-Play-On-Tape/SpaceCab_Pokitto", "",
    "License: GPL-3.0 (full text with the game data on the card).", "",
    "Font 3x5: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.", "",
    "Speech and level files are on the card (rocks/games/pokitto_spacecab/). "
    "The level designer is not included in this port.",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_spacecab", "Space Cab", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=thrust  Down=gear|Hold Sel+Up=menu"
};
