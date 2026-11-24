#pragma once

/* Rockbox port: the sound effect is a raw unsigned 8-bit 16 kHz file below
 * the game's data dir (.rockbox/rocks/games/pokitto_lightsout/), written by
 * tools/lightsout_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Switch = { "sfx/Switch.raw" };

};
