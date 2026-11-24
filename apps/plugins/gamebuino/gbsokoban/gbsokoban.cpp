/* Sokoban for Rockbox: the sketch (Sources/*.ino) built against the
 * Gamebuino META layer. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Push every crate onto a target mark in the warehouse; crates can only "
    "be pushed, one at a time. 57 levels.", "",
    "The arrows walk and push. SELECT (A) shows the level map and, on the "
    "title screen, starts the game; SELECT+DOWN (B) opens the restart "
    "prompt in play and goes back in menus. The in-game help is in French.", "",
    "Sound effects and clips mix with Rockbox music, which keeps playing. Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Sokoban", "",
    "Game by Jicehel; Gamebuino META tutorial version by Jean-Charles "
    "Lebeau.",
    "github.com/jicehel/Sokoban",
    "License: MIT (c) 2021 Jean-Charles Lebeau; full text in LICENSE.txt in "
    "rocks/games/gamebuino/gbsokoban/. Levels: sourcecode.se/sokoban/levels.",
    NULL
};

static const struct gb_rule keys[] = {
    {GB_P_SELECT | GB_P_DOWN, GB_B_B},
    {GB_P_SELECT,             GB_B_A},
    {GB_P_UP,                 GB_B_UP},
    {GB_P_DOWN,               GB_B_DOWN},
    {GB_P_LEFT,               GB_B_LEFT},
    {GB_P_RIGHT,              GB_B_RIGHT},
    {0, 0}
};

static uint16_t framebuffer[80 * 64] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "gbsokoban", "Sokoban", help, credits,
    keys, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 80, 64, (uint8_t *)framebuffer,
    "Arrows=walk  Sel=A (map)|Sel+Down=B (restart)|Hold Sel+Up=menu"
};

/* Streamed WAV clips (8-bit mono, in the game's card directory) play through
 * a small ring buffer; setup() hands it to the layer first. */
static uint8_t musicRing[4096];
#define setup sketchSetup
#include "Sokoban.ino"
#include "gbsokoban_proto.h"
#include "Graphics.ino"
#include "Help.ino"
#include "Level.ino"
#include "Player.ino"
#undef setup

void setup()
{
    gb_music_buffer(musicRing, sizeof musicRing);
    sketchSetup();
}
