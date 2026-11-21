#ifndef SWEET_VALENTINE_SPRITE
#define SWEET_VALENTINE_SPRITE

#include "Renderable.h"

// abstract class in charge of implementing the
// graphical rendering of all sprites in the game
// as well as the management of kinematic properties

class Sprite : public Renderable
{
    protected:

        // kinematic properties
        uint8_t  w,  h; // size
        int16_t  x,  y; // position
        int16_t vx, vy; // velocity
    
    public:

        // constructor
        Sprite(uint8_t w, uint8_t h, int16_t x, int16_t y, int16_t vx, int16_t vy);
        // pure virtual destructor (must be extended in derived classes)
        virtual ~Sprite() = 0;

        // position accessors
        int16_t getX();
        void setX(int16_t x);
        int16_t getY();
        void setY(int16_t y);

        // velocity accessors
        int16_t getVx();
        void setVx(int16_t x);
        int16_t getVy();
        void setVy(int16_t y);

        // transparent color accessor as pure virtual method (must be extended in derived classes)
        virtual const uint16_t getTransparentColor() = 0;
        // bitmap accessor as pure virtual method (must be extended in derived classes)
        virtual const uint16_t* getBitmap() = 0;
        
        // control loop entry point as pure virtual method (must be extended in derived classes)
        virtual void tick() = 0;
        // graphical rendering
        void draw(uint8_t sliceY, uint8_t sliceHeight, uint16_t* buffer);
};

#endif