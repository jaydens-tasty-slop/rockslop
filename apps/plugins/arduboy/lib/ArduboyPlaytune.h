#ifndef ARDUBOY_PLAYTUNE_H
#define ARDUBOY_PLAYTUNE_H
/* Rockbox port of the ArduboyPlaytune interface (Playtune score player, Len
 * Shustek / Arduboy-Playtune): two square-wave voices rendered by
 * ab_tunes_fill() on the mixer. The implementation is ArduboyPlaytune.cpp,
 * which a game lists in its sources (it defines ab_tunes_fill, so it cannot be
 * part of the common library). */
#include <Arduino.h>

#ifndef PIN_SPEAKER_1
#define PIN_SPEAKER_1 5
#endif
#ifndef PIN_SPEAKER_2
#define PIN_SPEAKER_2 13
#endif

class ArduboyPlaytune
{
 public:
  ArduboyPlaytune(bool (*outEn)());
  void initChannel(uint8_t) {}
  void playScore(const uint8_t *score);
  void stopScore();
  bool playing();
  void closeChannels();
  void setVolume(bool) {}
  /* Extras for OBONO's MyArduboy2 audio (lib/MyArduboy2Audio.inc): a score
   * with a semitone offset, and one extra square-wave tone voice. While the
   * tone plays the score can be muted. */
  void playScore(const uint8_t *score, int8_t pitch);
  void playTone(uint16_t hz, uint16_t ms);
  void stopTone();
  bool tonePlaying();
  void muteScore(bool mute);
  /* names of the original library's tone interface */
  void tone(uint16_t hz, uint32_t ms) { playTone(hz, ms); }
  void toneMutesScore(bool mute) { muteScore(mute); }
  void playWave(uint16_t hz, const uint8_t *wave, uint16_t samples); /* 8-bit unsigned */
  bool wavePlaying();
  void stopWave();
};
#endif
