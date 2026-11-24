/* Rockbox game description for GalaxyFighter. */
#include "Pokitto.h"
#include "src/Game.h"

extern Game game;

/* In play SELECT is A (fire) and the directions keep their meaning; C (pause)
 * is the pause menu. In the menus SELECT+LEFT is C (start, insert coin) and
 * SELECT+DOWN is B (high scores). */
static const struct pk_rule play_rules[] = {
    {PK_P_SELECT, PK_B_A},
    {PK_P_UP,     PK_B_UP},
    {PK_P_DOWN,   PK_B_DOWN},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {0, 0}
};
static const struct pk_rule menu_rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_B},
    {PK_P_SELECT | PK_P_LEFT, PK_B_C},
    {PK_P_SELECT,             PK_B_A},
    {PK_P_UP,                 PK_B_UP},
    {PK_P_DOWN,               PK_B_DOWN},
    {PK_P_LEFT,               PK_B_LEFT},
    {PK_P_RIGHT,              PK_B_RIGHT},
    {0, 0}
};
static const struct pk_keys play_keys = {play_rules, 0, 0, 0};
static const struct pk_keys menu_keys = {menu_rules, 0, 0, 0};

static const struct pk_keys *current_keys()
{
    return game.inPlay() ? &play_keys : &menu_keys;
}

static const char *const help[] = {
    "Defend the galaxy from waves of attacking aliens; rescue captured "
    "fighters and clear the bonus stages.", "",
    "Left/Right: move the fighter.", "",
    "Select: fire.", "",
    "Title screen: Select+Left starts the game, Select+Down shows the "
    "high scores. High score entry: Up/Down change a letter, Select "
    "confirms.", "",
    "Hold Select+Up for 0.5 s: pause menu.", "",
    "The images load into the music buffer, so the music player stops "
    "while it runs.",
    NULL
};

static const char *const credits[] = {
    "GalaxyFighter", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/GalaxyFighter_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_galaxyfighter/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_galaxyfighter", "GalaxyFighter", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=fire|Title: Sel+Left=start|Hold Sel+Up=menu"
};
