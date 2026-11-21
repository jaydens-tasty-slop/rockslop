/* Bang! Bang! for Rockbox: the Gamebuino Classic sketch (gbbangbang.ino) built
 * against the Gamebuino META layer through gbclassic.h. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Artillery duel: set your cannon's angle and force, then fire to dig "
    "into the terrain. Destroy the right-hand cannon to win.", "",
    "Up/Down raise or lower the cannon, Left/Right shorten or lengthen the "
    "shot. SELECT (A) fires and restarts after a win.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Bang! Bang!", "",
    "Gamebuino Classic artillery game by Thomas Pietrzak.",
    "github.com/ArduboyCollection/gamebuino-bangbang",
    "License: GPL-3.0; full text in LICENSE.txt in "
    "rocks/games/gamebuino/gbbangbang/.",
    NULL
};

static uint16_t framebuffer[84 * 48] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "gbbangbang", "Bang! Bang!", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 84, 48, (uint8_t *)framebuffer,
    "Arrows=aim  Sel=A fire|Hold Sel+Up=menu"
};

#include "gbbangbang.ino"
