/* Rockbox game description for RoadTrip. */
#include "Pokitto.h"

/* Holding SELECT is the throttle (A) and the directions steer and change
 * gear meanwhile. SELECT+DOWN is the brake (B, which the game only honours
 * with the throttle off). The radio (C) is not mapped. */
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
    "Drive as far as you can along the road without crashing into "
    "the traffic.", "",
    "Left/Right: steer.", "",
    "Select (hold): accelerate; let go to coast.", "",
    "Up/Down: change gear up or down.", "",
    "Select+Down: brake (the throttle must be off, so press Select+Down "
    "after releasing Select).", "",
    "Menus: Select confirms.", "",
    "Hold Select+Up for 0.5 s: pause menu.", "",
    "The images load into the music buffer, so the music player stops "
    "while it runs. The radio station button is not mapped.",
    NULL
};

static const char *const credits[] = {
    "RoadTrip", "",
    "Original game by Press Play On Tape (Filmote).", "",
    "https://github.com/Press-Play-On-Tape/RoadTrip_Pokitto", "",
    "License: BSD-3-Clause.", "",
    "Removed: the five tunes (no author or licence upstream), so there is no music.", "",
    "Font 4x7: PokittoLib (BSD-3-Clause).", "",
    "Images and sound effects are on the card (rocks/games/pokitto_roadtrip/).",
    NULL
};

extern "C" const struct pk_game pk_game = {
    "pokitto_roadtrip", "RoadTrip", help, credits, current_keys,
    PK_P_SELECT | PK_P_UP, 500, 1, "Sel=gas  Sel+Down=brake|Up/Down=gear|Hold Sel+Up=menu"
};
