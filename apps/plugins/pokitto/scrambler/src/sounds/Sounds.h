#pragma once

/* Rockbox port: the sound effects are raw unsigned 8-bit 16 kHz files below
 * the game's data dir (.rockbox/rocks/games/pokitto_scrambler/), written by
 * tools/scrambler_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Explosion_00 = { "sfx/Explosion_00.raw" };
    inline constexpr Audio::Clip sfx_Explosion_01 = { "sfx/Explosion_01.raw" };
    inline constexpr Audio::Clip sfx_Explosion_02 = { "sfx/Explosion_02.raw" };
    inline constexpr Audio::Clip sfx_PlayerBomb = { "sfx/PlayerBomb.raw" };
    inline constexpr Audio::Clip sfx_PlayerLaser = { "sfx/PlayerLaser.raw" };
    inline constexpr Audio::Clip sfx_RocketLaunch = { "sfx/RocketLaunch.raw" };
    inline constexpr Audio::Clip sfx_SurfaceToAir = { "sfx/SurfaceToAir.raw" };
    inline constexpr Audio::Clip sfx_FuelUp = { "sfx/FuelUp.raw" };
    inline constexpr Audio::Clip sfx_StageComplete = { "sfx/StageComplete.raw" };

};
