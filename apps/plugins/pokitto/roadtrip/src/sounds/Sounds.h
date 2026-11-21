#pragma once

/* Rockbox port: the sound effects are raw unsigned 8-bit 16 kHz files below
 * the game's data dir (.rockbox/rocks/games/pokitto_roadtrip/), written by
 * tools/roadtrip_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Bump = { "sfx/Bump.raw" };
    inline constexpr Audio::Clip sfx_ButtonPress = { "sfx/ButtonPress.raw" };
    inline constexpr Audio::Clip sfx_Passing = { "sfx/Passing.raw" };
    inline constexpr Audio::Clip sfx_Skid = { "sfx/Skid.raw" };
    inline constexpr Audio::Clip sfx_Beep = { "sfx/Beep.raw" };

}
