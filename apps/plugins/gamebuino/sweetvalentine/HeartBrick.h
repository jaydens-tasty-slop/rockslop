#ifndef SWEET_VALENTINE_HEART_BRICK
#define SWEET_VALENTINE_HEART_BRICK

#include <Gamebuino-Meta.h>
#include "Sprite.h"

class HeartBrick : public Sprite
{
    public:
    
        // constructor
        HeartBrick(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy);
        // destructor
        ~HeartBrick();

         // transparent color accessor
        const uint16_t getTransparentColor() override;
        // bitmap accessor
        const uint16_t* getBitmap() override;

        void tick();
};

#endif