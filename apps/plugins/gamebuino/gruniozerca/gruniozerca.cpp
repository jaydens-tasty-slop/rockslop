/* Gruniozerca for Rockbox: the sketch (gruniozerca-meta.ino) built against
 * the Gamebuino META layer. */
#include <Gamebuino-Meta.h>

static const char *const help[] = {
    "Catch the falling carrots with the guinea pig and avoid the parsnips. "
    "Three lives; the score is kept.", "",
    "LEFT and RIGHT walk. SELECT (A) starts the game and confirms; "
    "SELECT+LEFT (MENU) goes back to the title screen from the game-over "
    "screen. The game's music and sounds are streamed from the card and mix "
    "with Rockbox playback, which keeps playing.", "",
    "Hold SELECT+UP for 0.5 s for this pause menu.",
    NULL
};

static const char *const aboutText[] = {
    "Gruniozerca Meta", "",
    "Port of the NES game Gruniozerca. Code: Dark Archon (arhn.eu); "
    "graphics: Neko; concept: Lukasz Kur.",
    "github.com/arhneu/gruniozerca-meta",
    "Code and art: public domain (Unlicense).",
    "Music by Ozzed (ozzed.net), CC BY-SA 3.0, resampled to 22.05 kHz; "
    "the licence text is on the card.",
    NULL
};

static const struct gb_rule keys[] = {
    {GB_P_SELECT | GB_P_LEFT, GB_B_MENU},
    {GB_P_SELECT,             GB_B_A},
    {GB_P_UP,                 GB_B_UP},
    {GB_P_DOWN,               GB_B_DOWN},
    {GB_P_LEFT,               GB_B_LEFT},
    {GB_P_RIGHT,              GB_B_RIGHT},
    {0, 0}
};

static uint16_t framebuffer[80 * 64] __attribute__((aligned(4)));
static uint8_t musicRing[4096];

extern "C" const struct gb_game gb_game = {
    "gruniozerca", "Gruniozerca", help, aboutText,
    keys, 0, 0, 0, 0,
    GB_VIEW_FIT, GB_RGB565, 80, 64, (uint8_t *)framebuffer,
    "Left/Right=walk  Sel=A|Sel+Left=MENU|Hold Sel+Up=menu"
};

/* The sketch uses libc's rand(). */
#define rand() ((int)random(32768))

/* setup() hands the ring buffer to the layer first. */
#define setup sketchSetup
#include "gruniozerca-meta.ino"
#undef setup

void setup()
{
    gb_music_buffer(musicRing, sizeof musicRing);
    sketchSetup();
}
