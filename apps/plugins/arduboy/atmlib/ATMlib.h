/* ATMlib (Arduboy Tracker Music) for the Rockbox Arduboy layer: the upstream
 * interface; the AVR timer ISR is replaced by a PCM source (ATMlib.cpp).
 * (c) 2016 - 2018 TEAM a.r.g., Davey Taylor, Joeri Gantois, BSD-3-Clause
 * (LICENSE.ATMlib). */
#ifndef _ATMLIB_H_
#define _ATMLIB_H_

#include <stddef.h>
#include <inttypes.h>
#include <Arduino.h>

#define CH_ZERO             0
#define CH_ONE              1
#define CH_TWO              2
#define CH_THREE            3

class ATMsynth {
  public:
    ATMsynth() {};

    // Load and play specified song
    void play(const byte *song);

    // Play or Pause playback
    void playPause();

    // Stop playback (unloads song)
    void stop();

    void muteChannel(byte ch);
    void unMuteChannel(byte ch);
};

#endif
