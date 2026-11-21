/* Apollo for Rockbox: the sketch (Apollo.ino) built against the Gamebuino
 * META layer, in its 160x128 indexed-colour mode, scaled to the screen. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Land the lunar module softly on the flat pads: mind your fuel, your "
    "speed and the module's tilt.", "",
    "LEFT and RIGHT fire the side thrusters and tilt the module; SELECT (A) "
    "fires the main thruster while held. Hold SELECT with LEFT or RIGHT to "
    "thrust and tilt together. SELECT also starts and restarts the game. "
    "The potentiometer controls of the original are not available.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Apollo", "",
    "Game by Steph (Stephane Calderoni), inspired by Lunar Lander.",
    "github.com/m1cr0lab-gamebuino/apollo",
    "License: MIT (c) 2021 Stephane Calderoni; full text in LICENSE.txt in "
    "rocks/games/gamebuino/apollo/.",
    NULL
};

/* Thrust with a side thruster: SELECT+LEFT / SELECT+RIGHT. */
static const struct gb_rule keys[] = {
    {GB_P_SELECT | GB_P_LEFT,  GB_B_A | GB_B_LEFT},
    {GB_P_SELECT | GB_P_RIGHT, GB_B_A | GB_B_RIGHT},
    {GB_P_SELECT,              GB_B_A},
    {GB_P_UP,                  GB_B_UP},
    {GB_P_DOWN,                GB_B_DOWN},
    {GB_P_LEFT,                GB_B_LEFT},
    {GB_P_RIGHT,               GB_B_RIGHT},
    {0, 0}
};

static uint8_t framebuffer[160 * 128 / 2] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "apollo", "Apollo", help, credits,
    keys, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_INDEXED, 160, 128, framebuffer,
    "Left/Right=tilt  Sel=thrust|Sel+Left/Right=both|Hold Sel+Up=menu"
};

#include "Apollo.ino"
