/* Rockbox glue for the Flipper Flappy Bird (see ../README.rockbox). */
#include "flipper_game.h"

int32_t flappy_game_app(void *p);

static const char *const help[] = {
    "Fly the bird through the gaps between the pipes.",
    "",
    "SELECT or UP flaps. After a crash, SELECT starts again.",
    "Hold SELECT+UP for half a second to open this menu (Quit is here).",
    NULL,
};

static const char *const credits[] = {
    "Flappy Bird",
    "By @DroomOne and @xMasterX (Flipper plugin).",
    "github.com/xMasterX/all-the-plugins, base_pack/flappy_bird",
    "License: GPL v3 (the repository's; the app file has no own header).",
    "Full text: rocks/games/flappybird/LICENSE.txt on the card.",
    NULL,
};

const struct flipper_game flipper_game = {
    .name = "Flappy Bird",
    .entry = flappy_game_app,
    .help = help,
    .credits = credits,
    .hint = "Sel/Up=flap|Hold Sel+Up=menu",
    .data_dir = "/.rockbox/rocks/games/flappybird",
    .back = FLIPPER_BACK_MENU,
    .quit = FLIPPER_QUIT_SHORT,
    .boost = 1,
};
