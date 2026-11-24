/* Rockbox game description for Shattered Lands 2: Sea of Despair. A (cancel) and B (accept, map menu) are
 * the only buttons, B is used the most: SELECT = B, SELECT + DOWN = A. */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A role-playing saga: part two. Sail the sea, explore, and fight turn-based battles. CONTINUE starts a new game as HERO unless a save exists (part one cannot hand over its progress).", "",
    "Directions: walk, choose.", "",
    "Select: the B button (accept, open the menu on the map).", "",
    "Select + Down: the A button (cancel, back).", "",
    "Hold Select + Up for 0.5 s: pause menu. The title's UPLOAD item and the "
    "end-of-game upload do nothing here; QUIT GAME leaves the game.",
    NULL
};

static const char *const credits[] = {
    "Shattered Lands 2: Sea of Despair", "",
    "Justin (tuxinator2009) Davis, 2019-2020.", "",
    "https://github.com/tuxinator2009/ShatteredLands", "",
    "License: GPL-3.0 or later (code and art).",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "shatteredlands2", "Shattered Lands 2: Sea of Despair", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=B (OK)  Sel+Down=A|Hold Sel+Up=menu"
};
