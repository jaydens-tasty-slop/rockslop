/* Rockbox game description for PZero (Pokitto Grand Prix). */
#include "Pokitto.h"
#include "fix16.h"
#include "main.h"
#include "animation.h"
#include "menu.h"

extern CMenu menu;

/* Racing: SELECT is A (thrust, hold it), Down is B (brake/reverse), Up opens
 * the game's menu (C). In its menus Up/Down move and SELECT chooses. */
static const struct pk_rule race_rules[] = {
    {PK_P_SELECT, PK_B_A},
    {PK_P_UP,     PK_B_C},
    {PK_P_DOWN,   PK_B_B},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {0, 0}
};
static const struct pk_rule menu_rules[] = {
    {PK_P_SELECT, PK_B_A},
    {PK_P_UP,     PK_B_UP},
    {PK_P_DOWN,   PK_B_DOWN},
    {PK_P_LEFT,   PK_B_LEFT},
    {PK_P_RIGHT,  PK_B_RIGHT},
    {0, 0}
};
static const struct pk_keys race_keys = {race_rules, 0, 0, 0};
static const struct pk_keys menu_keys = {menu_rules, 0, 0, 0};

static const struct pk_keys *current_keys()
{
    return menu.m_isOpen ? &menu_keys : &race_keys;
}

static const char *const help[] = {
    "Race six computer-controlled ships on an F-Zero style track, or "
    "run a time trial.", "",
    "Left/Right: steer.", "",
    "Select (hold): accelerate.", "",
    "Down: brake and reverse.", "",
    "Up: open the game menu (track, pilot, laps). Up/Down move in the "
    "menu and Select chooses.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pokitto Grand Prix (PZero)", "",
    "Code: Hannu Viitala (Hanski). Apache-2.0.", "",
    "Textures and ships: Pharap, CC BY-NC-SA (non-commercial use only). "
    "Cars: Adekto, CC0. Title screen: jpfli. Number font: Vampirics.", "",
    "https://github.com/haviital/PokittoLib (branch pzero2, Examples/PokittoGP)", "",
    "Fixed-point maths: libfixmath, MIT.", "",
    "Custom tracks go in rocks/games/pokitto_pzero/pgpdata/tracks/. The "
    "game is silent but for the engine tone.",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_pzero", "Pokitto Grand Prix", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=gas  Down=brake|Up=track menu|Hold Sel+Up=menu"
};
