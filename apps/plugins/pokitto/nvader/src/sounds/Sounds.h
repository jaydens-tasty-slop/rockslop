#pragma once

/* Rockbox port: the sound effects are not linked into the plugin; each is a
 * raw unsigned 8-bit 16 kHz file below the game's data dir
 * (.rockbox/rocks/games/pokitto_nvader/), written by tools/nvader_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Drop_Bomb = { "sfx/drop_bomb.raw" };
    inline constexpr Audio::Clip sfx_Laser = { "sfx/Laser.raw" };
    inline constexpr Audio::Clip sfx_Mini_Explosion = { "sfx/Mini_Explosion.raw" };
    inline constexpr Audio::Clip sfx_Level_Cleared = { "sfx/Level_Cleared.raw" };
    inline constexpr Audio::Clip sfx_Bomb_HitGround = { "sfx/Bomb_HitGround.raw" };
    inline constexpr Audio::Clip sfx_Player_Hit = { "sfx/Player_Hit.raw" };

};
