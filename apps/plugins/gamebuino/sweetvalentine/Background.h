#ifndef SWEET_VALENTINE_BACKGROUND
#define SWEET_VALENTINE_BACKGROUND

#include <Gamebuino-Meta.h>
#include "Renderable.h"

class Background : public Renderable
{
    public:

        // constructor
        Background();
        // destructor
        ~Background();

        // graphical rendering
        void draw(uint8_t sliceY, uint8_t sliceHeight, uint16_t* buffer);
};

#endif