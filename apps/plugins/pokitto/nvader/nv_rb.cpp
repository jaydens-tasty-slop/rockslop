/* Rockbox game description for 1nvader. */
#include "Pokitto.h"

/* Any direction or SELECT fires the laser (A); SELECT+DOWN is B, which
 * flips the screen between landscape and portrait on the title screen. */
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
    "Defend the city from the invaders and push them back.", "",
    "Select or any direction key: fire.", "",
    "Title screen: Left/Right/Up/Down choose the mode and difficulty, "
    "Select starts.", "",
    "Title screen, Select+Down: turn the screen between landscape and "
    "portrait.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "1nvader", "",
    "Original game by Press Play On Tape (bloggingadeadhorse).", "",
    "https://github.com/Press-Play-On-Tape/1nvader_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_nvader/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_nvader", "1nvader", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel/dirs=fire|Sel+Down=rotate|Hold Sel+Up=menu"
};
