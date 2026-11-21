/* Minesweeper for Rockbox: the sketch (Minesweeper.ino) built against the
 * Gamebuino META layer, in its 160x128 indexed-colour mode; the layer turns
 * the screen to portrait, so the player is held sideways. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Uncover every square that hides no mine; a number tells how many mines "
    "touch the square. Flag the mines (you have as many flags as there are "
    "mines). 999 seconds, three levels (30, 50 or 70 mines).", "",
    "Hold the player turned clockwise: the arrows follow the screen. SELECT "
    "(A) uncovers a square and confirms; SELECT+DOWN (B) places or removes a "
    "flag and goes back; SELECT+LEFT (MENU) shows mines left and time.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Minesweeper", "",
    "Game by Steph (Stephane Calderoni), gamebuino.m1cr0lab.com.",
    "github.com/m1cr0lab-gamebuino/Minesweeper",
    "License: MIT (c) 2025 Stephane Calderoni; full text in LICENSE.txt in "
    "rocks/games/gamebuino/gbminesweeper/.",
    NULL
};

static const struct gb_rule keys[] = {
    {GB_P_SELECT | GB_P_DOWN, GB_B_B},
    {GB_P_SELECT | GB_P_LEFT, GB_B_MENU},
    {GB_P_SELECT,             GB_B_A},
    {GB_P_UP,                 GB_B_UP},
    {GB_P_DOWN,               GB_B_DOWN},
    {GB_P_LEFT,               GB_B_LEFT},
    {GB_P_RIGHT,              GB_B_RIGHT},
    {0, 0}
};

static uint8_t framebuffer[160 * 128 / 2] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "gbminesweeper", "Minesweeper", help, credits,
    keys, 0, 0, 0, 0,
    GB_VIEW_ROT90, GB_INDEXED, 160, 128, framebuffer,
    "Sel=dig  Sel+Down=flag|Sel+Left=info|Hold Sel+Up=menu"
};

/* The sketch only uses sin() for the LED effects, which the layer ignores:
 * a parabola for a in 0..2pi is plenty. */
static float sin(float a)
{
    float x = a > 3.14159265f ? a - 6.2831853f : a;
    return x * (1.2732395f - 0.4052847f * (x < 0 ? -x : x));
}

#include "Minesweeper.ino"
