/* UFO Race for Rockbox: the sketch (*.ino, main file first as the Arduino IDE
 * orders them) built against the Gamebuino META layer. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Race the UFO round the track against the clock; sand and grass slow you down.", "",
    "LEFT and RIGHT steer. Hold SELECT (A) to accelerate: keep it held "
    "while steering. SELECT+DOWN (B) brakes. The game's own menu lists "
    "START and the high scores; entering a name uses UP/DOWN for the "
    "letter and SELECT or RIGHT for the next one.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "UFO Race", "",
    "Game by Rodot.",
    "github.com/Rodot/UFO-Race-META",
    "License: LGPL-3.0; full text in LICENSE.txt in "
    "rocks/games/gamebuino/uforace/.",
    NULL
};

/* A (gas) is held with SELECT while LEFT/RIGHT steer. */
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
    "uforace", "UFO Race", help, credits,
    keys, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 80, 64, (uint8_t *)framebuffer,
    "Left/Right=steer  Sel=gas|Sel+Down=brake|Hold Sel+Up=menu"
};

#include "uforace_proto.h"
#include "UFO-Race.ino"
#include "highscore.ino"
#include "menu.ino"
#include "player.ino"
#include "time.ino"
#include "world.ino"
