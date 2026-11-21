/* Rockbox game description for Cursed Mansion 1300. Select = A (use item, confirm); Sel+Down = B (strike a match, dark mode). D-pad moves, diagonals allowed. */
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
    "Find the three pieces of the urn and escape the mansion, avoiding "
    "the ghost, the giant spider and the bats.", "",
    "D-pad: move. Select (A): use the item you carry (the scepter "
    "protects from spiders and bats). Select+Down (B): light a match in "
    "dark mode. Choose difficulty, lights and bats in the start screen.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Cursed Mansion 1300",
    "",
    "ImMrShrike (Cursed Mansion 1300), a recreation of Haunted House.",
    "",
    "https://github.com/ImMrShrike/Cursed-Mansion-1300 @ 9089962",
    "",
    "License: AGPL 3. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "cursedmansion", "Cursed Mansion 1300", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "D-pad=move  Sel=use|SelDn=match|Hold Sel+Up=menu"
};
