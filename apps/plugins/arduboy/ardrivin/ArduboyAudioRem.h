#ifndef ARDUBOY_AUDIO_H
#define ARDUBOY_AUDIO_H

#include <Arduino.h>
#include <EEPROM.h>

/* The game's own sound switch; the plugin's Sound setting is a master switch
 * on top of it (enabled() is false when either is off). */
class ArduboyAudioRem
{
 public:
  void static begin();
  void static on();
  void static off();
  void static saveOnOff() {}
  bool static enabled();

 protected:
  bool static audio_enabled;
};

#endif
