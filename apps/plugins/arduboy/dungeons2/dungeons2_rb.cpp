/* Rockbox game description for Dungeons 2. B slashes/confirms (SELECT), SELECT+DOWN = A+B (inventory, back), SELECT+LEFT = A (skips to the next level). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_A | AB_B_B},
    {AB_P_SELECT | AB_P_LEFT, AB_B_A},
    {AB_P_SELECT,             AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "A top-down dungeon crawler prototype with generated levels.", "",
    "Directions: walk.", "",
    "Select: slash (B) and confirm in menus.", "",
    "Select + Down: A+B together: inventory and back.", "",
    "Select + Left: A: skip to the next level (a developer shortcut "
    "upstream).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Dungeons 2",
    "",
    "TEAM a.r.g. (2016): Gaveno, Jo3ri, Castpixel.",
    "",
    "https://github.com/TEAMarg/Dungeons2 (ID-21) @ d79ab43",
    "",
    "Code and art: MIT licence, (c) TEAM a.r.g. Work in progress upstream; no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "dungeons2", "Dungeons 2", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=slash  Sel+Down=A+B|Sel+Left=A|Hold Sel+Up=menu"
};
