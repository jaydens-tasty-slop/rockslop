/* Rockbox game description for Ardu-Whack. Select = A (smash, confirm); D-pad moves the hammer. */
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
    "Whack-a-mole: smash every mole that pops up, but spare the bad "
    "ones, for the highest score.", "",
    "D-pad: move the hammer. Select: smash. The start menu has PLAY, "
    "SOUND and INFO.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ardu-Whack",
    "",
    "Code, graphics and sound by zeduckmaster "
    "(framagit.org/zeduckmaster/ardu-whack).",
    "",
    "https://framagit.org/zeduckmaster/ardu-whack @ d01b9c9 (mirror: "
    "ArduboyCollection)",
    "",
    "License: GPL 3. Sounds through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "arduwhack", "Ardu-Whack", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "D-pad=move  Sel=smash|Hold Sel+Up=menu"
};
