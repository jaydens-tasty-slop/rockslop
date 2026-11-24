#include <Arduino.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "snd.h"

extern Arduboy2 arduboy;
static ArduboyTones tones(arduboy.audio.enabled);

void SndInit(void) {}
// The original drives two timer channels; both go through ArduboyTones.
void SndPlay(u8 ch, u16 freq, u16 duration)
{
	(void)ch;
	if (freq) tones.tone(freq, duration);
}
void SndStartTimer(u8 ch, u32 freq) { (void)ch; if (freq) tones.tone((uint16_t)freq, 20); }
void SndStopTimer(u8 ch) { (void)ch; }
