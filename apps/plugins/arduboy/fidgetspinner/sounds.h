#ifndef SOUNDS_H
#define SOUNDS_H

#include "globals.h"
#include <ArduboyTones.h>
#include <ArduboyTonesPitches.h>

#define SOUNDS_SPINNING NOTE_A5

// Plays a spinning sound based on the speed of the spinner
void sounds_playSpinSound(byte fidgetSpinnerSpeedPercentage) {
  // Check the speed of the spinner
  if (fidgetSpinnerSpeedPercentage > 77) {
    // The spinner is spinning at a fast speed
    if (arduboy.everyXFrames(FRAMERATE / 10)) {
      sound.tone(SOUNDS_SPINNING, 5);
    }
  } else if (fidgetSpinnerSpeedPercentage > 33) {
    // The spinner is spinning at a medium speed
    if (arduboy.everyXFrames(FRAMERATE / 6)) {
      sound.tone(SOUNDS_SPINNING, 20);
    }
  } else if (fidgetSpinnerSpeedPercentage > 0) {
    // The spinner is spinning at a slow speed
    if (arduboy.everyXFrames(FRAMERATE / 1.5)) {
      sound.tone(SOUNDS_SPINNING, 40);
    }
  } else if (sound.playing()) {
    // The spinner is not spinning but the sound is still playing.
    sound.noTone();
  }
}

#endif
