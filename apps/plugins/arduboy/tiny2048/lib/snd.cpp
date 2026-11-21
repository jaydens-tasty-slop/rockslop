#include <Arduino.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "snd.h"

extern Arduboy2 arduboy;
static ArduboyTones tones(arduboy.audio.enabled);

void SndInit(void) {}
void SndPlayScore(const u8* p) { (void)p; }
void SndStopScore(void) {}
void SndStepScore(void) {}
void SndPlayNote(u8 ch, u8 note) { (void)ch; (void)note; }
void SndStopNote(u8 ch) { (void)ch; }
void SndPlayTone(u16 freq, u32 duration) { if (freq) tones.tone(freq, duration); }
void SndStopTone(void) {}
void SndStartTimerCh0(void) {}
void SndStartTimerCh(u8 ch, u32 freq) { (void)ch; (void)freq; }
void SndStopTimer(u8 ch) { (void)ch; }
