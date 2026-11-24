#ifndef SWEET_VALENTINE_RENDERER
#define SWEET_VALENTINE_RENDERER

#include <Gamebuino-Meta.h>
#include "Renderable.h"
#include "constants.h"

// the rendering engine obeys the *observable* pattern
// and has a collection of *observers* who have all
// subscribed to the `Renderable` contract

class Renderer
{
    private:

        // the number of registered observers
        static uint8_t numListeners;
        // the collection of *observers*
        static Renderable* listeners[MAX_NUM_RENDERABLE];
    
        // declaration of the two memory buffers in which
        // will be performed the rendering calculations
        static uint16_t buffer1[SCREEN_WIDTH * RENDERING_SLICE_HEIGHT];
        static uint16_t buffer2[SCREEN_WIDTH * RENDERING_SLICE_HEIGHT];
        // flag which will tell us if a memory forwarding
        // to the DMA controller is in progress or not
        static bool drawPending;

        // method to initiate memory forwarding to the DMA controller
        static void customDrawBuffer(int16_t x, int16_t y, uint16_t* buffer, uint16_t w, uint16_t h);
        // method for waiting for the transfer to be completed
        // and close the transaction with the DMA controller
        static void waitForPreviousDraw();
    
    public:

        // observer subscription
        static void addListener(Renderable* listener);
        // unsubscribing an observer
        static void removeListener(Renderable* listener);

        // performs graphical rendering of the game scene
        static void draw();
};

#endif
