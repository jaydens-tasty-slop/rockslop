/* Rockbox game description for Catacombs of the Damned. */
#include <Arduino.h>

/* In the original, B fires and A + LEFT/RIGHT strafes. SELECT fires;
 * SELECT + LEFT/RIGHT strafes. */
static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LEFT,  AB_B_A | AB_B_LEFT},
    {AB_P_SELECT | AB_P_RIGHT, AB_B_A | AB_B_RIGHT},
    {AB_P_SELECT,              AB_B_B},
    {AB_P_UP,                  AB_B_UP},
    {AB_P_DOWN,                AB_B_DOWN},
    {AB_P_LEFT,                AB_B_LEFT},
    {AB_P_RIGHT,               AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Explore ten floors of a dungeon, shoot monsters and collect loot.", "",
    "Up/Down: walk forward/back.", "",
    "Left/Right: turn.", "",
    "Select: fire a fireball (also confirms in menus).", "",
    "Select + Left/Right: strafe.", "",
    "Hold Select + Up for 1 s: pause menu (Select + Up is also fire while "
    "walking, hence the longer hold).",
    NULL
};

static const char *const credits[] = {
    "Catacombs of the Damned", "",
    "Original game by James Howard (jhhoward), Arduboy3D.", "",
    "https://github.com/jhhoward/Arduboy3D", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "catacombs", "Catacombs of the Damned", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 1000, 0, 0, 0, NULL, 0,
    "Sel=fire  Sel+L/R=strafe|Hold Sel+Up=menu"
};
