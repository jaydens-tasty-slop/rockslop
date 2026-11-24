/* Rockbox game description for Ring Puzzle. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "A ring rotating puzzle game.", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ring Puzzle", "",
    "Skuratovich Sergey.", "",
    "https://github.com/ArduboyCollection/ring_puzzle_game", "",
    "License: MIT.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "ringpuzzle", "Ring Puzzle", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
