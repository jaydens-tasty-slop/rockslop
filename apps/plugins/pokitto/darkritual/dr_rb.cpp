/* Rockbox game description for Dark Ritual. */
#include "Pokitto.h"
#include "src/Game.h"

extern Game game;

/* Walking around: SELECT taps shoot (B), holding SELECT acts (A: levers,
 * doors, stairs), SELECT+DOWN opens the inventory (C). In menus a tap
 * confirms (A) and a hold goes back (B). */
static const struct pk_rule rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_C},
    {PK_P_UP,                 PK_B_UP},
    {PK_P_DOWN,               PK_B_DOWN},
    {PK_P_LEFT,               PK_B_LEFT},
    {PK_P_RIGHT,              PK_B_RIGHT},
    {0, 0}
};
static const struct pk_keys play_keys = {rules, PK_B_B, PK_B_A, 450};
static const struct pk_keys menu_keys = {rules, PK_B_A, PK_B_B, 450};

static const struct pk_keys *current_keys()
{
    return game.getGameState() == GameState::Game ? &play_keys
                                                               : &menu_keys;
}

static const char *const help[] = {
    "Explore the dark underworld to find all the sacred runes.", "",
    "Direction keys: walk (and aim).", "",
    "Select (tap): shoot.", "",
    "Select (hold half a second): act - pull levers, open doors, "
    "use stairs, read guides.", "",
    "Select+Down: open the inventory.", "",
    "In menus: Select taps confirm, a Select hold goes back.", "",
    "Hold Select+Up for 0.5 s: pause menu.", "",
    "The game's images load into the music buffer, so playback stops "
    "while it runs.",
    NULL
};

static const char *const credits[] = {
    "Dark Ritual", "",
    "Code: Filmote. Graphics: Vampirics. Music: raphaelespindola and "
    "Vampirics. Published by Press Play On Tape.", "",
    "https://github.com/Press-Play-On-Tape/DarkRitual_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Font 3x5: Gamebuino, (c) 2014 Aurelien Rodot, LGPL-3.0 or later.", "",
    "Sound files are on the card (rocks/games/pokitto_darkritual/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_darkritual", "Dark Ritual", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=shoot  hold=act|Sel+Down=inventory|Hold Sel+Up=menu"
};
