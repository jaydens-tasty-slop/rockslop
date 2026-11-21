/* PongMETA for Rockbox: the sketch (PongMETA.ino) built against the
 * Gamebuino META layer. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Pong. The computer plays both paddles until you move yours: take over "
    "the left paddle with UP and DOWN. First to score wins the round. The "
    "right paddle (A and B in the original) is left to the computer.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Pong", "",
    "Game by Rodot (PongMETA), the Gamebuino META demo.",
    "github.com/Rodot/PongMETA",
    "License: LGPL-3.0; full text in LICENSE.txt in "
    "rocks/games/gamebuino/gbpong/.",
    NULL
};

static uint16_t framebuffer[80 * 64] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "gbpong", "Pong", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 80, 64, (uint8_t *)framebuffer,
    "Up/Down=paddle|Hold Sel+Up=menu"
};

#include "PongMETA.ino"
