/* Solitaire for Rockbox: the Gamebuino Classic sketch built against the
 * Gamebuino META layer through gbclassic.h. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Klondike solitaire. Build the four foundations up by suit from ace "
    "to king; tableau piles alternate colours, descending.", "",
    "Arrows move the cursor. SELECT (A) picks up and drops; SELECT+DOWN "
    "(B) opens the game menu. Statistics are saved.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const credits[] = {
    "Solitaire", "",
    "Gamebuino Classic Solitaire by Andy O'Neill.",
    "github.com/ArduboyCollection/gamebuino-solitaire",
    "License: MIT; full text in LICENSE.txt in "
    "rocks/games/gamebuino/gbsolitaire/.",
    NULL
};

static uint16_t framebuffer[84 * 48] __attribute__((aligned(4)));

extern "C" const struct gb_game gb_game = {
    "gbsolitaire", "Solitaire", help, credits,
    NULL, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 84, 48, (uint8_t *)framebuffer,
    "Arrows=cursor  Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};

#include "gbsolitaire.ino"
