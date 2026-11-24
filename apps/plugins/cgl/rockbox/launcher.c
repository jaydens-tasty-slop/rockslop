/***************************************************************************
 * Crisp Game Lib Portable for Rockbox: game list.  Each game is its own
 * plugin (cgl/<name>.rock) so that it fits the plugin arena; this plugin
 * only lists them and chain-loads the chosen one.
 * Games and library: MIT, (c) 2022 ABA Games.
 ***************************************************************************/
#include "plugin.h"

static const struct {
    const char *title;
    const char *rock;
} games[] = {
    { "BALL TOUR",   "balltour" },
    { "CAST N",      "castn" },
    { "COLOR ROLL",  "colorroll" },
    { "FRACAVE",     "fracave" },
    { "FROOOOG",     "froooog" },
    { "GRAPPLING H", "grapplingh" },
    { "HEXMIN",      "hexmin" },
    { "IN TOW",      "intow" },
    { "LADDER DROP", "ladderdrop" },
    { "PAKU PAKU",   "pakupaku" },
    { "PIN CLIMB",   "pinclimb" },
    { "R WHEEL",     "rwheel" },
    { "REFLECTOR",   "reflector" },
    { "SURVIVOR",    "survivor" },
    { "THUNDER",     "thunder" },
    { "TIMBER TEST", "timbertest" },
};
#define GAME_COUNT ((int)(sizeof(games) / sizeof(games[0])))
#define ABOUT_ITEM GAME_COUNT

static const char *about_lines[] = {
    "Crisp Game Lib",
    "Portable, games by",
    "ABA Games (Kenta Cho)",
    "github.com/abagames/",
    "crisp-game-lib-portable",
    "License: MIT",
    "(c) 2022 ABA Games",
    "Rockbox port: YP3",
    "Full text: LICENSE.txt",
    "in rocks/games/cgl/",
    "",
    "SELECT: game button",
    "Arrows: directions",
    "SELECT+UP (hold):",
    "  pause menu",
};

static bool about_mode;
static bool chosen;

static const char *get_name(int item, void *data, char *buf, size_t len)
{
    (void)data; (void)buf; (void)len;
    if (about_mode)
        return about_lines[item];
    return item == ABOUT_ITEM ? "About" : games[item].title;
}

static int action_cb(int action, struct gui_synclist *lists)
{
    (void)lists;
    if (!about_mode && action == ACTION_STD_OK) {
        chosen = true;
        return ACTION_STD_CANCEL;
    }
    return action;
}

enum plugin_status plugin_start(const void *param)
{
    (void)param;
    struct simplelist_info info;
    int last = 0;
    for (;;) {
        about_mode = false;
        chosen = false;
        rb->simplelist_info_init(&info, "Crisp Game Lib", GAME_COUNT + 1, NULL);
        info.get_name = get_name;
        info.action_callback = action_cb;
        info.selection = last;
        if (rb->simplelist_show_list(&info))
            return PLUGIN_USB_CONNECTED;
        if (!chosen)
            return PLUGIN_OK;
        last = info.selection;
        if (last == ABOUT_ITEM) {
            about_mode = true;
            rb->simplelist_info_init(&info, "About",
                                     sizeof(about_lines) / sizeof(*about_lines),
                                     NULL);
            info.get_name = get_name;
            if (rb->simplelist_show_list(&info))
                return PLUGIN_USB_CONNECTED;
            continue;
        }
        char path[MAX_PATH];
        rb->snprintf(path, sizeof(path), PLUGIN_GAMES_DIR "/cgl/cgl_%s.rock",
                     games[last].rock);
        return rb->plugin_open(path, NULL);
    }
}
