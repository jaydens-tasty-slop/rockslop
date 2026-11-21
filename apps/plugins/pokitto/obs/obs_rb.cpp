/* Rockbox game description for OBS - One Button Shooter. */
#include "Pokitto.h"

/* SELECT is the one button (fire, continue); SELECT+DOWN is the C button,
 * held on the score screen to clear the high scores. */
static const struct pk_rule rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_C},
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
    "Steer your ship home through asteroids and hostile ships.", "",
    "Select: fire the laser. Firing also changes your direction.", "",
    "Up/Down on the title screen: sound (both, music, effects, off).", "",
    "On the score screen hold Select+Down to clear the high scores.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "OBS - One Button Shooter", "",
    "Original game by Press Play On Tape (bloggingadeadhorse).", "",
    "https://github.com/Press-Play-On-Tape/OBS_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Sound data is extracted to the card (rocks/games/obs/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_obs", "One Button Shooter", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=fire|Hold Sel+Up=menu"
};
