#include "ArduboyRem.h"
#include "ArduboyAudioRem.h"

bool ArduboyAudioRem::audio_enabled = false;

void ArduboyAudioRem::on()
{
  audio_enabled = true;
}

void ArduboyAudioRem::off()
{
  audio_enabled = false;
}

void ArduboyAudioRem::begin()
{
}

bool ArduboyAudioRem::enabled()
{
  return audio_enabled && ab_audio_enabled();
}
