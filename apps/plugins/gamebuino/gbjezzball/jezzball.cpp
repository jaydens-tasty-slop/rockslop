/* JezzBall for Rockbox: the Gamebuino Classic sketch (jezzball.ino) built
 * against the Gamebuino META layer through gbclassic.h. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Build walls to confine the bouncing balls: claim 75% of the board to "
    "clear the level. A wall that is hit by a ball while growing is lost "
    "and costs a life.", "",
    "Arrows move the cursor. SELECT (A) builds a wall; SELECT+DOWN (B) "
    "switches its direction. The high score is saved.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "JezzBall", "",
    "Gamebuino Classic JezzBall by Thomas Pietrzak, a tribute to "
    "the TI-89 game.",
    "github.com/ArduboyCollection/gamebuino-jezzball",
    "License: GPL-3.0; full text in LICENSE.txt in "
    "rocks/games/gamebuino/gbjezzball/.",
    NULL
};

static uint16_t framebuffer[84 * 48] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "gbjezzball", "JezzBall", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 84, 48, (uint8_t *)framebuffer,
    "Arrows=cursor  Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};

/* Stand-ins for the sketch's floor(log10(n)) and Classic gb.collidePointRect. */
static inline int jb_log10f(int n) {   /* floor(log10(|n|)), n != 0 */
    int d = 0;
    if (n < 0) n = -n;
    while (n >= 10) { n /= 10; d++; }
    return d;
}
static inline bool gb_pointrect(int16_t px, int16_t py, int16_t x, int16_t y, int16_t w, int16_t h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

#include "jezzball.ino"
