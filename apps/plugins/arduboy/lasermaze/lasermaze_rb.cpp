/* Rockbox game description for Laser Maze. Select = A (select / move / rotate a mirror), Select+Down = B (laser on/off). */
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
    "Place mirrors so the laser beam hits every target; 70 levels.", "",
    "Directions: move the cursor.", "",
    "Select: pick up, place or rotate a mirror (A).", "",
    "Select + Down: laser on/off (B); a thumbs up moves to the next "
    "level.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Laser Maze",
    "",
    "Frank van de Ven (2026), after the board game Laser Maze.",
    "",
    "https://github.com/FrankVanDeVen/LaserMaze @ 46615d2",
    "",
    "License: MIT. No sound upstream. The level is kept in the per-game "
    "save file.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "lasermaze", "Laser Maze", help, credits, keys,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=mirror  Sel+Down=laser|Hold Sel+Up=menu"
};
