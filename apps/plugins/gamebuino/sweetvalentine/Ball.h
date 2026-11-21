#ifndef SWEET_VALENTINE_BALL
#define SWEET_VALENTINE_BALL

#include <Gamebuino-Meta.h>
#include "Sprite.h"

class Ball : public Sprite
{
    public:
    
        // constructor
        Ball(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy);
        // destructor
        ~Ball();

         // transparent color accessor
        const uint16_t getTransparentColor() override;
        // bitmap accessor
        const uint16_t* getBitmap() override;

        void tick();
        void start();
};

#endif