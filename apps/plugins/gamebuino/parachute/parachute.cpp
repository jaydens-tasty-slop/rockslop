/* Game & Watch Parachute for Rockbox: the sketch (Parachute_Meta.ino) built
 * against the Gamebuino META layer. The game streams 160x8 pixel slices to
 * the TFT; the layer turns them to portrait, so the player is held sideways. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Catch the parachutists dropped by the helicopter in your boat before "
    "they reach the sharks. Three misses end the game.", "",
    "Hold the player turned clockwise: LEFT and RIGHT steer the boat along "
    "the screen. SELECT (A) starts the game and pauses or resumes it.", "",
    "Sound effects and clips mix with Rockbox music, which keeps playing. "
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Parachute", "",
    "Game by Jicehel, after Nintendo's Game & Watch Parachute.",
    "github.com/jicehel/Parachute_Meta",
    "License: MIT (c) 2019 Stephane Calderoni; full text in LICENSE.txt in "
    "rocks/games/gamebuino/parachute/.",
    NULL
};

extern "C" const struct gb_game gb_game = {
    "parachute", "Parachute", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_ROT90, GB_RGB565, 160, 128, NULL,
    "Hold turned clockwise|Left/Right=move  Sel=A|Hold Sel+Up=menu"
};

/* Streamed WAV clips (8-bit mono, in the game's card directory) play through
 * a small ring buffer; setup() hands it to the layer first. */
static uint8_t musicRing[4096];
#define setup sketchSetup
#include "Parachute_Meta.ino"
#undef setup

void setup()
{
    gb_music_buffer(musicRing, sizeof musicRing);
    sketchSetup();
}
