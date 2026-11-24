/* Rockbox game description for Ball Bust. */
#include "Pokitto.h"

/* SELECT is A (jump, kick the ball); SELECT+DOWN is B (start, pause, restart). */
static const struct pk_rule rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_B},
    {PK_P_SELECT,             PK_B_A},
    {PK_P_UP,                 PK_B_UP},
    {PK_P_DOWN,               PK_B_DOWN},
    {PK_P_LEFT,               PK_B_LEFT},
    {PK_P_RIGHT,              PK_B_RIGHT},
    {0, 0}
};

static const struct pk_keys keys = {rules, 0, 0, 0};
static const struct pk_keys *current_keys() { return &keys; }

static const char *const help[] = {
    "Kick the ball into the enemies and stay alive.", "",
    "Left/Right: run.", "",
    "Select: jump (hold to jump higher), kick the ball.", "",
    "Select+Down: start the game, pause it, play again after game over.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Ball Bust", "",
    "Original game by Gitonym.", "",
    "https://github.com/Gitonym/Ball-Bust", "",
    "License: Unlicense (public domain).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_ballbust", "Ball Bust", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=jump|Sel+Down=start/pause|Hold Sel+Up=menu"
};
