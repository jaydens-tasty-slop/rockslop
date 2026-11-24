/* Rockbox game description for L'Abbaye des Morts. */
#include "Pokitto.h"

/* SELECT is A (jump), Down ducks, Up is C (pause). The Rockbox menu is the
 * SELECT+UP chord. */
static const struct pk_rule rules[] = {
    {PK_P_SELECT, PK_B_A},
    {PK_P_UP,     PK_B_C},
    {PK_P_DOWN,   PK_B_DOWN},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {0, 0}
};
static const struct pk_keys keys = {rules, 0, 0, 0};
static const struct pk_keys *current_keys() { return &keys; }

static const char *const help[] = {
    "Jean Raymond, a Cathar, hides in an old abbey. Find the twelve "
    "crosses and escape.", "",
    "Left/Right: run. Down: duck.", "",
    "Select: jump (steer in the air).", "",
    "Up: pause and read the hints.", "",
    "The screen follows Jean in a window; the title and story screens are "
    "scaled down.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "L'Abbaye des Morts", "",
    "Game by Locomalito, graphics by Gryzor87, music by Gryzor87.", "",
    "Linux port David Lara (nevat); Pokitto port HomineLudens, FManga, "
    "drummyfish, jonne.", "",
    "https://gitlab.com/HomineLudens/abbaye-des-morts-pokitto", "",
    "Code: GPL-3.0. Graphics and music: CC BY 3.0. The licence texts are on "
    "the card (rocks/games/pokitto_abbaye/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_abbaye", "L'Abbaye des Morts", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 0, "Sel=jump  Up=pause|Hold Sel+Up=menu"
};
