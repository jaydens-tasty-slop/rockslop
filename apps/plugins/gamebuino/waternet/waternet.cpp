/* Waternet (Gamebuino META) for Rockbox: the sketch built against the
 * Gamebuino META layer (indexed 160x128 display, note-track music). */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Connect all the pipes so water flows from the source through every "
    "one of them. Rotate mode: press A on a pipe to turn it. Slide mode: "
    "press A on the arrows of a row or column to shift it. Pick the mode "
    "and read the rules in the main menu's help.", "",
    "Hold the player turned clockwise: the arrows follow the screen. "
    "Arrows move the cursor or selection. SELECT (A) confirms, turns or "
    "slides. SELECT+DOWN (B) goes back and pauses; press both keys "
    "together and let go together. Progress, level locks and sound "
    "options are saved. Music and effects are the game's own tones, mixed "
    "with Rockbox playback.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Waternet", "",
    "Game and music by Willems Davy (joyrider3774), after Net and Netslide "
    "by Simon Tatham.",
    "github.com/joyrider3774/waternet_gamebuino_meta",
    "License: MIT (c) 2022 Willems Davy; full text in LICENSE.txt in "
    "rocks/games/gamebuino/waternet/.",
    NULL
};

static uint8_t framebuffer[160 * 128 / 2] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "waternet", "Waternet", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_ROT90, GB_INDEXED, 160, 128, framebuffer,
    "Hold turned clockwise|Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};

#include "waternet.ino"
