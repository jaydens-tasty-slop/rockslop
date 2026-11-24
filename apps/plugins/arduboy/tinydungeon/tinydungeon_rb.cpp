/* Rockbox game description for Tiny Dungeon. Select = A (fire: attack, open, use); D-pad: Up/Down move, Left/Right turn. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "First-person dungeon crawler in the style of Dungeon Master: find "
    "the Fountain of Life.", "",
    "Up/Down: step forward or back (backing away flees a fight). "
    "Left/Right: turn. Select: attack, open doors and chests, use "
    "switches.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Tiny Dungeon",
    "",
    "Sven B (Lorandil), 2021-2026. Inspired by TinyJoypad (Daniel C).",
    "",
    "https://github.com/Lorandil/TinyDungeon @ 1f0f60e",
    "",
    "License: MIT. Wall style inspired by Eye of the Beholder, art "
    "drawn by the author. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "tinydungeon", "Tiny Dungeon", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Up/Dn=move  Lt/Rt=turn|Sel=act/attack|Hold Sel+Up=menu"
};
