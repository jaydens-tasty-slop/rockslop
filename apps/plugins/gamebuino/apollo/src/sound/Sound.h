/**
 * -------------------------------------------------------------------------
 *                                  Apollo
 * -------------------------------------------------------------------------
 *         a tiny game for the Gamebuino META retro gaming handheld
 *                    inspired by the famous Lunar Lander
 *                       https://youtu.be/McAhSoAEbhM
 *       https://en.wikipedia.org/wiki/Lunar_Lander_(1979_video_game)
 * -------------------------------------------------------------------------
 *                          © 2021 Steph @ m1cr0lab
 *                       https://gamebuino.m1cr0lab.com
 * -------------------------------------------------------------------------
 */

#pragma once

#include <Gamebuino-Meta.h>

constexpr Gamebuino_Meta::Sound_FX tick_fx[] = {{Gamebuino_Meta::Sound_FX_Wave::SQUARE, 0, 128, -128, 0, 5, 1}};

class ApolloSound {  // (Sound upstream: clashes with the layer's)

    private:

        uint32_t _last_play;

    public:

        void tick(uint8_t delay = 0);

};