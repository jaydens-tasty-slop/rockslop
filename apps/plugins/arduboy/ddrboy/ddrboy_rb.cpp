/* Rockbox game description for DDRBOY. Select = A (start/confirm), Select+Down = B (sound; hold 3 s to reset the high score). */
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
    "A rhythm game: press the matching button as each note scrolls past "
    "in time with the beat.", "",
    "Up, Down, Left, Right: the four arrow notes.", "",
    "Select: the A note (also starts the game).", "",
    "Select + Down: the B note (also toggles sound on the title; hold "
    "it 3 s there to reset the high score).", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "DDRBOY",
    "",
    "Tyler Nijmeh (tytydraco), 2022.",
    "",
    "https://github.com/tytydraco/DDRBOY @ 1b0b98e",
    "",
    "License: BSD 2-Clause. Sound through ArduboyTones.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ddrboy", "DDRBOY", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Dir=arrows  Sel=A|Hold Sel+Up=menu"
};
