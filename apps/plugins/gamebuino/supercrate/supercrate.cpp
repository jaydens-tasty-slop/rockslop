/* Super Crate Box (SuperCrateMETA) for Rockbox: the sketch built against the
 * Gamebuino META layer. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Collect the crates for new weapons and shoot the enemies; do not let "
    "them reach the lava at the bottom.", "",
    "LEFT and RIGHT run. UP jumps (B in the original; press again in the "
    "air for the double jump). SELECT (A) fires, hold it to keep firing "
    "and to pick menu entries. The weapon and map unlocks are saved.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Super Crate Box", "",
    "Game by Aurelien Rodot, after Super Crate Box by Vlambeer.",
    "github.com/Rodot/SuperCrateMETA",
    "License: LGPL-3.0; full text in LICENSE.txt in "
    "rocks/games/gamebuino/supercrate/.",
    NULL
};

/* UP is the jump button (B): the game does not use UP otherwise. */
static const struct gb_rule keys[] = {
    {GB_P_SELECT,             GB_B_A},
    {GB_P_UP,                 GB_B_B},
    {GB_P_DOWN,               GB_B_DOWN},
    {GB_P_LEFT,               GB_B_LEFT},
    {GB_P_RIGHT,              GB_B_RIGHT},
    {0, 0}
};

static uint16_t framebuffer[80 * 64] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "supercrate", "Super Crate Box", help, credits,
    keys, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 80, 64, (uint8_t *)framebuffer,
    "Left/Right=run  Up=jump|Sel=fire (hold)|Hold Sel+Up=menu"
};

#include "supercrate_proto.h"
#include "SuperCrateMeta.ino"
