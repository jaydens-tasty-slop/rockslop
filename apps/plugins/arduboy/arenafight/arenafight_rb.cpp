/* Rockbox game description for Arena Fight. Select = A (confirm), Select+Down = B (back); directions move. */
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
    "A turn-based arena fighter: pick a class, buy equipment and fight "
    "through the arena to the bosses.", "",
    "Directions: move the cursor.", "",
    "Select: confirm (A).", "",
    "Select + Down: B (starts the game on the title; back / special in "
    "menus).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Arena Fight",
    "",
    "FreezingSnail (2018).",
    "",
    "https://github.com/FreezingSnail/fight @ 9f3a5a2",
    "",
    "License: MIT. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arenafight", "Arena Fight", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=move  Sel=A  SelDn=B|Hold Sel+Up=menu"
};
