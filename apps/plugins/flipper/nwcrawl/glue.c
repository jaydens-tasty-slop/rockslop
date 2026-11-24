/* Rockbox glue for rseufert's NW Crawl (see ../README.rockbox). */
#include "flipper_game.h"
#include <input/input.h>

int32_t nw_crawl_app(void *p);

static const char *const help[] = {
    "A tiny roguelike on the alphabet streets of NW Portland. Walk north,",
    "one street per floor, to the Witch's Castle.",
    "",
    "Direction keys move; walk into a local to attack it. Tap SELECT to",
    "drink a coffee or wait a turn. Hold SELECT, then press a direction, to",
    "swing the umbrella or fire the slingshot (hold SELECT again for the",
    "street map and character sheet; any key closes it).",
    "Hold SELECT+UP for half a second to open this menu.",
    NULL,
};

static const char *const credits[] = {
    "NW Crawl",
    "Copyright (c) 2026 rseufert.",
    "github.com/rseufert/nw_crawl",
    "License: MIT. Full text: rocks/games/nwcrawl/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "NW Crawl",
    .entry = nw_crawl_app,
    .help = help,
    .credits = credits,
    .hint = "Sel=wait  Hold Sel=aim|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/nwcrawl",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_LONG,
    .sel_tap_key = InputKeyOk + 1,
    .sel_long_key = InputKeyBack + 1,
    .sel_long_ms = 400,
};
