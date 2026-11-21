/* Playtune-based audio of the bundled Arduboy v1 library, reimplemented for
 * Rockbox: same interface, output through the mixer (ab_glue.c). */
#ifndef ArduboyAudio_h
#define ArduboyAudio_h

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

#define AVAILABLE_TIMERS 2
#define TUNE_OP_PLAYNOTE  0x90  /* play a note: low nibble is generator #, note is next byte */
#define TUNE_OP_STOPNOTE  0x80  /* stop a note: low nibble is generator # */
#define TUNE_OP_RESTART   0xe0  /* restart the score from the beginning */
#define TUNE_OP_STOP      0xf0  /* stop playing */

class ArduboyAudio
{
public:
  void static begin();
  void static on();
  void static off();
};

class ArduboyTunes
{
public:
  void initChannel(byte pin, byte chan);
  void playScore(const byte *score);
  void stopScore();
  void delay(unsigned msec);
  void closeChannels();
  bool playing();
  void tone(unsigned int frequency, unsigned long duration);

  void static playNote (byte chan, byte note);
  void static stopNote (byte chan);
};
#endif
