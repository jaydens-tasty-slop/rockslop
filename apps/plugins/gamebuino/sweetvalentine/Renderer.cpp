#include "Renderer.h"
#include "constants.h"

uint8_t Renderer::numListeners = 0;
Renderable* Renderer::listeners[MAX_NUM_RENDERABLE];
// for (uint8_t i=0; i<MAX_NUM_RENDERABLE; i++) Renderer::listeners[i] = NULL;

// definition of the two memory buffers in which
// will be performed the rendering calculations
uint16_t Renderer::buffer1[SCREEN_WIDTH * RENDERING_SLICE_HEIGHT];
uint16_t Renderer::buffer2[SCREEN_WIDTH * RENDERING_SLICE_HEIGHT];

// for the moment, no memory forwarding is in progress
bool Renderer::drawPending = false;

// the magic routine related to the DMA controller...
// take a look at the official library if it arouses your curiosity:
// Gamebuino-META/src/utility/Display-ST7735/Display-ST7735.cpp
namespace Gamebuino_Meta {
  #define DMA_DESC_COUNT (3)
  extern volatile uint32_t dma_desc_free_count;

  static inline void wait_for_transfers_done(void) {
    while (dma_desc_free_count < DMA_DESC_COUNT);
  }

  static SPISettings tftSPISettings = SPISettings(24000000, MSBFIRST, SPI_MODE0);
};

// method to initiate memory forwarding to the DMA controller....
// these operations are also defined in:
// Gamebuino-META/src/utility/Display-ST7735/Display-ST7735.cpp
void Renderer::customDrawBuffer(int16_t x, int16_t y, uint16_t* buffer, uint16_t w, uint16_t h) {
  drawPending = true;
  gb.tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
  SPI.beginTransaction(Gamebuino_Meta::tftSPISettings);
  gb.tft.dataMode();
  gb.tft.sendBuffer(buffer, w*h);
}

// method to wait for the transfer to be completed
// and to close the transaction with the DMA controller....
// Ditto, it's all in:
// Gamebuino-META/src/utility/Display-ST7735/Display-ST7735.cpp
void Renderer::waitForPreviousDraw() {
  if (drawPending) {
    // the little magic routine defined above
    Gamebuino_Meta::wait_for_transfers_done();
    gb.tft.idleMode();
    SPI.endTransaction();
    drawPending = false;
  }
}

// performs graphical rendering of the game scene
void Renderer::draw() {
    // the number of horizontal slices to be cut is calculated
    uint8_t slices = SCREEN_HEIGHT / RENDERING_SLICE_HEIGHT;
    // then we go through each slice one by one
    for (uint8_t sliceIndex = 0; sliceIndex < slices; sliceIndex++) {

        // we define a pointer that will alternate between the two memory buffers
        uint16_t* buffer = sliceIndex % 2 == 0 ? buffer1 : buffer2;
        // the ordinate of the first horizontal fringe of the slice is calculated
        uint8_t sliceY = sliceIndex * RENDERING_SLICE_HEIGHT;

        // a notification is sent to all observers to ask them to
        // graphically render their appearance on the game scene
        for (uint8_t i=0; i<numListeners; i++) {
            listeners[i]->draw(sliceY, RENDERING_SLICE_HEIGHT, buffer);
        }
    
        // then we make sure that the sending of the previous buffer
        // to the DMA controller has taken place
        if (sliceIndex != 0) waitForPreviousDraw();
        // after which we can then send the current buffer
        customDrawBuffer(0, sliceY, buffer, SCREEN_WIDTH, RENDERING_SLICE_HEIGHT);
    }

    // always wait until the DMA transfer is completed.
    // for the last slice before leaving the method!
    waitForPreviousDraw();
}

// observer subscription
void Renderer::addListener(Renderable* listener) {
  if (numListeners < MAX_NUM_RENDERABLE) {
      listener->setIndex(numListeners);
      listeners[numListeners++] = listener;
  }
}

// unsubscribing an observer
void Renderer::removeListener(Renderable* listener) {
    int8_t index = listener->getIndex();
    listener->setIndex(-1);

    uint8_t lastindex = numListeners - 1;
    size_t i;
    if (index == lastindex) {
        listeners[lastindex] = NULL;
        numListeners--;
        return;
    }

    for (i=index; i<lastindex; i++) {
        listeners[i] = listeners[i+1];
        listeners[i]->setIndex(i);
    }
    listeners[lastindex] = NULL;
    numListeners--;
}