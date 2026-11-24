#pragma once

/* Rockbox port: the two sound effects are raw unsigned 8-bit 16 kHz files
 * below the game's data dir (.rockbox/rocks/games/pokitto_fireworks/),
 * written by tools/fireworks_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Fireworks = { "sfx/Fireworks.raw" };
    inline constexpr Audio::Clip sfx_Whistle = { "sfx/Whistle.raw" };

};
