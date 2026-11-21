/* Sweet Valentine for Rockbox: the sketch (SweetValentine.ino) built against
 * the Gamebuino META layer. The game streams 160x8 pixel slices to the TFT;
 * the layer turns them to portrait, so the player is held sideways. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "A small inverted breakout: break the wall of hearts with the ball "
    "using the paddle.", "",
    "Hold the player turned clockwise: LEFT and RIGHT move the paddle along "
    "the screen. SELECT (A) starts the game from the title screen and "
    "launches the ball.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Sweet Valentine", "",
    "Game by Stephane Calderoni (Steph).",
    "github.com/m1cr0lab-gamebuino/gb-sweet-valentine",
    "License: MIT (c) 2019 Stephane Calderoni; full text in LICENSE.txt in "
    "rocks/games/gamebuino/sweetvalentine/.",
    NULL
};

extern "C" const struct gb_game gb_game = {
    "sweetvalentine", "Sweet Valentine", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_ROT90, GB_RGB565, 160, 128, NULL,
    "Hold turned clockwise|Left/Right=paddle  Sel=A|Hold Sel+Up=menu"
};

#include "SweetValentine.ino"
