#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <ArduboyTonesPitches.h>

#define FRAMERATE 60 // The target framerate

Arduboy2 arduboy; // Used for Arduboy-related stuff
ArduboyTones sound(arduboy.audio.enabled); // Enable sound
Sprites sprites; // Used for Arduboy sprite-related stuff

#endif
