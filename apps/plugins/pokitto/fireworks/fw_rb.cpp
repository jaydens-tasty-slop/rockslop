/* Rockbox game description for FireWorks. */
#include "Pokitto.h"

/* A (select a rocket, confirm) is a SELECT tap; DOWN is B (detonate the
 * selected rockets); C (pause) is the pause menu. */
static const struct pk_rule rules[] = {
    {PK_P_SELECT | PK_P_LEFT,  PK_B_B | PK_B_LEFT},
    {PK_P_SELECT | PK_P_RIGHT, PK_B_B | PK_B_RIGHT},
    {PK_P_UP,                  PK_B_UP},
    {PK_P_DOWN,                PK_B_B},
    {PK_P_LEFT,                PK_B_LEFT},
    {PK_P_RIGHT,               PK_B_RIGHT},
    {0, 0}
};

static const struct pk_keys keys = {rules, PK_B_A, 0, 0};
static const struct pk_keys *current_keys() { return &keys; }

static const char *const help[] = {
    "Aim the launcher at a rocket and select it; select a chain of "
    "rockets of matching colour or shape, then set them off.", "",
    "Left/Right: aim one degree (hold to keep turning).", "",
    "Up: aim 15 degrees.", "",
    "Select (tap): select the rocket in the line of fire.", "",
    "Down: detonate the selected rockets.", "",
    "Select+Left/Right: aim 15 degrees the other way.", "",
    "Hold Select+Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "FireWorks", "",
    "Original game by Press Play On Tape (bloggingadeadhorse).", "",
    "https://github.com/Press-Play-On-Tape/FireWorks_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Sound data is extracted to the card (rocks/games/pokitto_fireworks/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_fireworks", "FireWorks", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 0, "Sel=pick  Down=fire|Hold Sel+Up=menu"
};
