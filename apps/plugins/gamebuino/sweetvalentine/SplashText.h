#ifndef SWEET_VALENTINE_SPLASH_TEXT
#define SWEET_VALENTINE_SPLASH_TEXT

#include <Gamebuino-Meta.h>
#include "Sprite.h"

class SplashText : public Sprite
{
    private:

        // remains true throughout the duration of the movement  
        bool easing;
        // target position
        int16_t targetY;
        // easing coefficient
        float easing_factor;
        // friction coefficient
        float friction;
        
        // applies a damped movement between the current position and a target position
        void applyEasing();
    
    public:

        // constructor
        SplashText(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy);
        // destructor
        ~SplashText();

        // transparent color accessor
        const uint16_t getTransparentColor() override;
        // bitmap accessor
        const uint16_t* getBitmap() override;
        // set the target position
        void moveToY(int16_t y);
        // control loop entry point
        void tick();
};

#endif