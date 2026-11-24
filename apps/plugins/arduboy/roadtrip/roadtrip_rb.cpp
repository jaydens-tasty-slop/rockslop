/* Rockbox game description for Road Trip. Default mapping: SELECT = A,
 * SELECT + DOWN = B, directions as is. */
#include <Arduino.h>

static const char *const help[] = {
    "A pseudo-3D driving game.", "",
    "Directions: move, choose.", "",
    "Select: the A button.", "",
    "Select + Down: the B button.", "",
    "Hold Select + Up for 0.5 s: pause menu.",
    NULL
};

static const char *const credits[] = {
    "Road Trip", "",
    "Press Play On Tape.", "",
    "https://github.com/Press-Play-On-Tape/RoadTrip", "",
    "License: BSD-3-Clause.",
    NULL
};

extern "C" const struct ab_game ab_game = {
    "roadtrip", "Road Trip", help, credits, NULL,
    AB_P_SELECT | AB_P_UP, 500, 0, 0, 0, NULL, 0,
    "Sel=A  Sel+Down=B|Hold Sel+Up=menu"
};
