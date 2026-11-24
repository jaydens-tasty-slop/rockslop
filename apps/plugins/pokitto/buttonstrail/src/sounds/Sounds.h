#pragma once

/* Rockbox port: the tones are raw unsigned 8-bit 11.025 kHz files below the
 * game's data dir (.rockbox/rocks/games/pokitto_buttonstrail/), written by
 * tools/buttonstrail_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Tone_00 = { "sfx/Tone_00.raw" };
    inline constexpr Audio::Clip sfx_Tone_01 = { "sfx/Tone_01.raw" };
    inline constexpr Audio::Clip sfx_Tone_02 = { "sfx/Tone_02.raw" };
    inline constexpr Audio::Clip sfx_Tone_03 = { "sfx/Tone_03.raw" };
    inline constexpr Audio::Clip sfx_Tone_04 = { "sfx/Tone_04.raw" };
    inline constexpr Audio::Clip sfx_Tone_05 = { "sfx/Tone_05.raw" };
    inline constexpr Audio::Clip sfx_Tone_06 = { "sfx/Tone_06.raw" };
    inline constexpr Audio::Clip sfx_Tone_07 = { "sfx/Tone_07.raw" };
    inline constexpr Audio::Clip sfx_Tone_08 = { "sfx/Tone_08.raw" };
    inline constexpr Audio::Clip sfx_Tone_09 = { "sfx/Tone_09.raw" };

};
