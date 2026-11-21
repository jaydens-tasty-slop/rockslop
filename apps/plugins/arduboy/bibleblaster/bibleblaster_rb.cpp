/* Rockbox game description for Billy Graham's Bible Blaster. SELECT = B , SELECT + DOWN = A (reload). */
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
    "Lob bibles at the targets before they reach you.", "",
    "Left / Right: aim.  Up / Down: lob farther or nearer.", "",
    "Select: throw the bible in your hand.", "",
    "Select + Down: reload (six bibles).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Billy Graham's Bible Blaster", "",
    "jaychugstein, github.com/jaychugstein/Billy-Grahams-Bible-Blaster", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "bibleblaster", "Billy Graham's Bible Blaster", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=throw  Sel+Down=reload|Sel+Up=menu"
};
