/* Rockbox game description for Dark & Under. Default key mapping:
 * SELECT = A (confirm, map), SELECT + DOWN = B (back, inventory). */
#include <Arduino.h>

static const char *const help[] = {
    "A first-person dungeon crawler: explore ten levels, fight monsters "
    "turn by turn, find keys, potions, scrolls, shields and swords.", "",
    "Left/Right: turn. Up/Down: step forward/back; in menus move the "
    "selection.", "",
    "Select: the A button (confirm, fight, map overview).", "",
    "Select + Down: the B button (back, inventory).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Dark & Under", "",
    "Simon Holmes (Filmote) and contributors (2017).", "",
    "https://github.com/Press-Play-On-Tape/Dark-And-Under", "",
    "License: BSD-3-Clause (code and graphics as published in that "
    "repository).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "darkunder", "Dark & Under", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A (OK)  Sel+Down=B|Hold Sel+Up=menu"
};
