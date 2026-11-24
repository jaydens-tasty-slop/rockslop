#ifndef SWEET_VALENTINE_PADDLE
#define SWEET_VALENTINE_PADDLE

#include <Gamebuino-Meta.h>
#include "Sprite.h"

class Paddle : public Sprite
{
    private:

        // acceleration
        int16_t ax, ay;
        // remains true throughout the duration of the movement  
        bool easing;
        // target position
        int16_t targetX;
        // easing coefficient
        float easing_factor;
        // friction coefficient
        float friction;

        // applies a damped movement between the current position and a target position
        void applyEasing();

    public:
    
        // constructor
        Paddle(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy);
        // destructor
        ~Paddle();

         // transparent color accessor
        const uint16_t getTransparentColor() override;
        // bitmap accessor
        const uint16_t* getBitmap() override;

        // control loop entry point
        void tick();
        // move to left
        void left();
        // move to right
        void right();
};

#endif