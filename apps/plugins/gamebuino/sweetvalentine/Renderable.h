#ifndef SWEET_VALENTINE_RENDERABLE
#define SWEET_VALENTINE_RENDERABLE

#include <Gamebuino-Meta.h>

// this abstract class defines a kind of *interface* (a contract) :
// it guarantees that any object having subscribed to this contract
// is able to render its graphic appearance (by implementing the `draw` method)

class Renderable
{
    protected:

        int8_t index = -1;

    public:

        // pure virtual destructor (must be extended in derived classes)
        virtual ~Renderable() = 0;

        int8_t getIndex();
        void setIndex(int8_t index);

        // pure virtual method of graphical rendering
        virtual void draw(uint8_t sliceY, uint8_t sliceHeight, uint16_t* buffer) = 0;
};

#endif