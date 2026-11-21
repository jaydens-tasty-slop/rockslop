/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "button.h"
#include "buttonmap.h"
#include "SDL.h"
int key_to_button(int key)
{
    switch (key) {
    case SDLK_UP: return BUTTON_UP;
    case SDLK_DOWN: return BUTTON_DOWN;
    case SDLK_LEFT: return BUTTON_LEFT;
    case SDLK_RIGHT: return BUTTON_RIGHT;
    case SDLK_RETURN:
    case SDLK_SPACE: return BUTTON_SELECT;
    default: return BUTTON_NONE;
    }
}
struct button_map bm[] = {
    { SDLK_UP, 64, 25, 10, "Up" },
    { SDLK_DOWN, 64, 125, 10, "Down" },
    { SDLK_LEFT, 25, 80, 10, "Left" },
    { SDLK_RIGHT, 103, 80, 10, "Right" },
    { SDLK_RETURN, 64, 80, 10, "Center" },
    { 0, 0, 0, 0, "None" }
};
