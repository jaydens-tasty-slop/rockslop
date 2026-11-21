/* Octopus Game & Watch for Rockbox: the sketch (Octopus.ino) built against
 * the Gamebuino META layer. The game streams 160x8 pixel slices to the TFT;
 * the layer turns them to portrait, so the player is held sideways. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Dive from the boat to the treasure at the sea bottom and bring the "
    "gold back, without being caught by the octopus's tentacles.", "",
    "Hold the player turned clockwise: LEFT and RIGHT move the diver along "
    "the screen. SELECT (A) starts the game and pauses or resumes it.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Octopus", "",
    "Game by Jean-Charles Lebeau, after the Arduboy Octopus by Roby "
    "(rferrari81) and Game & Watch Octopus.",
    "github.com/jicehel/Octopus-for-META",
    "License: MIT (c) 2019 Stephane Calderoni; full text in LICENSE.txt in "
    "rocks/games/gamebuino/octopus/.",
    NULL
};

extern "C" const struct gb_game gb_game = {
    "octopus", "Octopus", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_ROT90, GB_RGB565, 160, 128, NULL,
    "Hold turned clockwise|Left/Right=move  Sel=A|Hold Sel+Up=menu"
};

#include "Octopus.ino"
