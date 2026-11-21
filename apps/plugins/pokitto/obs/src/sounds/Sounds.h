#pragma once

/* Rockbox port: the six sound effects are not linked into the plugin; each
 * is a raw unsigned 8-bit 16 kHz file below the game's data dir
 * (.rockbox/rocks/games/obs/), written by tools/obs_data.py. */
#include <LibAudio>

namespace Sounds {

    inline constexpr Audio::Clip sfx_Laser = { "sfx/Laser.raw" };
    inline constexpr Audio::Clip sfx_Mini_Explosion = { "sfx/Mini_Explosion.raw" };
    inline constexpr Audio::Clip sfx_Player_Hit = { "sfx/Player_Hit.raw" };
    inline constexpr Audio::Clip sfx_Game_Over = { "sfx/Game_Over.raw" };
    inline constexpr Audio::Clip sfx_Typewriter = { "sfx/Typewriter.raw" };
    inline constexpr Audio::Clip sfx_Health = { "sfx/Health.raw" };

};
