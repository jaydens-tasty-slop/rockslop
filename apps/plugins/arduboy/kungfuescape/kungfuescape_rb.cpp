/* Rockbox game description for Kung Fu Escape. In the game A = kick (a flying
 * kick when running), B = punch, and with Down held they are the crouching
 * kick and punch. SELECT tapped = B, SELECT held = A; with Down the same
 * (Down stays pressed). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_DOWN | AB_P_LONG, AB_B_A | AB_B_DOWN},
    {AB_P_SELECT | AB_P_DOWN,             AB_B_B | AB_B_DOWN},
    {AB_P_SELECT | AB_P_LONG,             AB_B_A},
    {AB_P_TAP,                            AB_B_B},
    {AB_P_UP,                             AB_B_UP},
    {AB_P_DOWN,                           AB_B_DOWN},
    {AB_P_LEFT,                           AB_B_LEFT},
    {AB_P_RIGHT,                          AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Fight your way through the stick figure gang to the exit.", "",
    "Left/Right: run.", "",
    "Down: crouch.", "",
    "Tap Select: punch.", "",
    "Hold Select for 0.4 s: kick (a flying kick while running).", "",
    "Select + Down: crouching punch; hold it for a crouching kick.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Kung Fu Escape", "",
    "MrSazazel.", "",
    "https://github.com/MrSazazel/Kung_Fu_Escape", "",
    "ArdBitmap by Ignacio Vina (Apache-2.0).", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "kungfuescape", "Kung Fu Escape", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 400, 120, 0, NULL, 0,
    "Tap Sel=punch Hold=kick|Sel+Down=crouch Sel+Up=menu"
};
