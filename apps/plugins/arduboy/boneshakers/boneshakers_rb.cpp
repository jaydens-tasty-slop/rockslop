/* Rockbox game description for Bone Shakers. Select = B (accelerate/start), Select+Down = A. */
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
    "Skeletons on unicycles race around a dungeon track, in the spirit "
    "of Super Mario Kart. Made for the Unofficial Arduboy game jam of "
    "September 2018.", "",
    "Left/Right: steer.", "",
    "Select: accelerate / start (B).", "",
    "Select + Down: A button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Bone Shakers",
    "",
    "James Howard (jhhoward), 2018.",
    "",
    "https://github.com/jhhoward/BoneShakers @ ab61ef8",
    "",
    "License: GPL 3. The game has no sound.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "boneshakers", "Bone Shakers", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "L/R=steer  Sel=go|Hold Sel+Up=menu"
};
