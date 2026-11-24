/* Arduboy2Audio for Rockbox: the on/off state is the plugin's Sound setting
 * (start menu), not an EEPROM byte. Original: Arduboy2 (BSD-3-Clause). */
#include "Arduboy2.h"

bool Arduboy2Audio::audio_enabled = false;

void Arduboy2Audio::on()
{
  audio_enabled = true;
  ab_audio_set(1);
}

void Arduboy2Audio::off()
{
  audio_enabled = false;
  ab_audio_set(0);
}

void Arduboy2Audio::toggle()
{
  if (audio_enabled)
    off();
  else
    on();
}

void Arduboy2Audio::saveOnOff()
{
}

void Arduboy2Audio::begin()
{
  audio_enabled = ab_audio_enabled();
}

bool Arduboy2Audio::enabled()
{
  return audio_enabled;
}
