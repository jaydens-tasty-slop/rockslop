#pragma once

/* Rockbox port: the sound effects are not linked into the plugin; each is a
 * raw unsigned 8-bit 11.025 kHz file below the game's data dir
 * (.rockbox/rocks/games/pokitto_minirogue/), written by
 * tools/minirogue_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_EvilLaugh = { "sfx/EvilLaugh.raw" };
    inline constexpr Audio::Clip sfx_RollDice = { "sfx/RollDice.raw" };
    inline constexpr Audio::Clip sfx_Positive = { "sfx/Positive.raw" };
    inline constexpr Audio::Clip sfx_Negative = { "sfx/Negative.raw" };
    inline constexpr Audio::Clip sfx_FistPunch1 = { "sfx/FistPunch1.raw" };
    inline constexpr Audio::Clip sfx_FistPunch2 = { "sfx/FistPunch2.raw" };
    inline constexpr Audio::Clip sfx_Welcome = { "sfx/Welcome.raw" };

};
