#ifndef SWEET_VALENTINE_TINY_HEART
#define SWEET_VALENTINE_TINY_HEART

#include <Gamebuino-Meta.h>
#include "Sprite.h"

class TinyHeart : public Sprite
{
    private:

        uint8_t rank;
        bool    dying;
        bool    dead;
    
    public:

        // constructor
        TinyHeart(uint8_t rank, uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy);
        // destructor
        ~TinyHeart();

        // transparent color accessor
        const uint16_t getTransparentColor() override;
        // bitmap accessor
        const uint16_t* getBitmap() override;
        // control loop entry point
        void tick();
        // makes sprite dying (quickly falling down the screen)
        void die();
};

#endif