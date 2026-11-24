/* Rockbox game description for Oh Mummy. Tap SELECT = B (shoot, music on/off on
 * the title), hold SELECT = A (open a chest, continue). */
#include <Arduino.h>

static const struct ab_rule keys[] = {
    {AB_P_SELECT | AB_P_LONG, AB_B_A},
    {AB_P_TAP,                AB_B_B},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static const char *const help[] = {
    "Walk the pyramid and open every chest to find the key to the exit, "
    "while the mummies hunt you.", "",
    "Directions: walk.", "",
    "Tap Select: shoot (B); on the title screen it toggles the music.", "",
    "Hold Select (0.3 s): open a chest (A) while walking over it; "
    "continue on the title screen.", "",
    "Hold Select + Up for 1 s: pause menu (the 0.3 s hold is the A button plus Up).",
    NULL
};

static const char *const credits[] = {
    "Oh Mummy",
    "",
    "Matthew Vicaradge (2019).",
    "",
    "https://github.com/Mattvic79/OhMummy",
    "",
    "License: BSD-3-Clause. The music is played through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ohmummy", "Oh Mummy", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 1000, 300, 0, 0, NULL, 0,
    "Tap=shoot  Hold=open chest|Hold Sel+Up=menu"
};
