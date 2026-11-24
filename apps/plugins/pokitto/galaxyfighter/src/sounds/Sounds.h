#pragma once

/* Rockbox port: the sound effects are raw unsigned 8-bit 8 kHz files below
 * the game's data dir (.rockbox/rocks/games/pokitto_galaxyfighter/), written
 * by tools/galaxyfighter_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Coin = { "sfx/Coin.raw" };
    inline constexpr Audio::Clip sfx_NewLife = { "sfx/NewLife.raw" };
    inline constexpr Audio::Clip sfx_Shoot = { "sfx/Shoot.raw" };
    inline constexpr Audio::Clip sfx_SequenceStart = { "sfx/SequenceStart.raw" };
    inline constexpr Audio::Clip sfx_ExtraLife = { "sfx/ExtraLife.raw" };

    const uint8_t sfx_Coin_Priority = 1;
    const uint8_t sfx_NewLife_Priority = 4;
    const uint8_t sfx_Shoot_Priority = 1;
    const uint8_t sfx_SequenceStart_Priority = 2;
    const uint8_t sfx_ExtraLife_Priority = 5;

}
