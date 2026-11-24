#ifndef HD_TOOLS
#define HD_TOOLS

#include "Global.h"
#include "Sprites.h"
#include "pictures.h"

// -------------------------------------------------------------------------
// Initialization related to the DMA controller
// -------------------------------------------------------------------------

static constexpr uint8_t DigitWidth      = 5; // width of each digit's sprite
static constexpr uint8_t DigitHeight     = 9; // height of each digit's sprite
static constexpr uint8_t DigitOffset     = 2; // offset in pixels between two digits
static constexpr uint8_t DigitSheetWidth = 10 * DigitWidth; // total width of the digits spritesheet

namespace Gamebuino_Meta {

#define DMA_DESC_COUNT 3
extern volatile uint32_t dma_desc_free_count;

static inline void wait_for_transfers_done(void) {
  while (dma_desc_free_count < DMA_DESC_COUNT);
}
static SPISettings tftSPISettings = SPISettings(24000000, MSBFIRST, SPI_MODE0);
};

// rendering buffers
uint16_t buffer1[screenWidth * sliceHeight];
uint16_t buffer2[screenWidth * sliceHeight];

// flag for an ongoing data transfer
bool drawPending = false;


void drawSprite(Sprite sprite, uint8_t sliceY, uint16_t* buffer) {
  // Check if sprite has one part to show on the current slice
  if (sliceY < sprite.y + sprite.h && sprite.y < sliceY + sliceHeight) {
    // Determines the boundaries of the sprite surface within the current slice
    uint8_t  xMin = sprite.x;
    uint8_t  xMax = sprite.x + sprite.w - 1;
    uint8_t  yMin = sprite.y < sliceY ? sliceY : sprite.y;
    uint8_t  yMax = sprite.y + sprite.h >= sliceY + sliceHeight ? sliceY + sliceHeight - 1 : sprite.y + sprite.h - 1;
    uint8_t  px, py;
    uint16_t color;
    // Display the sprite pixels to be drawn
    uint16_t row[screenWidth];
    for (py = yMin; py <= yMax; py++) {
      picRead(row, PIC_SHEET, xMin, py, xMax - xMin + 1);
      for (px = xMin; px <= xMax; px++) {
        // Picks the pixel color from the spritesheet
        color = row[px - xMin];
        // If color is the color selected for the Sprite
        if (color == sprite.color) {
          // Copies the color code into the rendering buffer
          buffer[px + (py - sliceY) * screenWidth] = sBlack;
        }
      }
    }
  }
}


void drawText(Sprite sprite, uint8_t sliceY, uint16_t* buffer, uint8_t x, uint8_t y) {
  if ((sliceY < (y + sprite.h)) && (y < (sliceY + sliceHeight))) {
    uint8_t xMin = x;
    uint8_t xMax = x + sprite.w - 1;
    uint8_t yMin = y < sliceY ? sliceY : y;
    uint8_t yMax = y + sprite.h >= sliceY + sliceHeight ? sliceY + sliceHeight - 1 : y + sprite.h - 1;

    uint8_t  px, py, sx, sy;
    uint16_t color;

    uint16_t row[screenWidth];
    for (py = yMin; py <= yMax; ++py) {
      sy = py - y + sprite.y;
      picRead(row, PIC_SHEET, sprite.x, sy, sprite.w);
      for (px = xMin; px <= xMax; ++px) {
        color = row[px - xMin];
        if (color == sRed) {
          buffer[px + (py - sliceY) * screenWidth] = sBlack;
        }
      }
    }
  }
}

void drawDigitOnSlice(uint8_t digit, uint8_t index, uint8_t sliceY, uint16_t * buffer) {
  // calculates the coordinates of the sprite
  uint8_t dx = 119 - index * (DigitWidth + DigitOffset);
  uint8_t dy = 6;

  // we check first of all that the intersection between
  // the sprite and the current slice is not empty
  if (sliceY < dy + DigitHeight && dy < sliceY + sliceHeight) {
    // determines the boundaries of the sprite surface within the current slice
    uint8_t ymin = dy < sliceY ? sliceY : dy;
    uint8_t ymax = dy + DigitHeight >= sliceY + sliceHeight ? sliceY + sliceHeight - 1 : dy + DigitHeight - 1;
    // determines the memory address of the byte block to be copied
    uint16_t * source = (uint16_t *)DIGIT_COLORMAP + digit * DigitWidth;
    // determines the destination memory address in the framebuffer
    uint16_t * dest = buffer + dx;
    // the 16-bit color codes are copied byte by byte,
    // so a factor of 2 is required here
    uint8_t size = DigitWidth << 1;
    // then each memory block is scanned
    for (uint8_t py = ymin; py <= ymax; py++) {
      // and copied to the framebuffer
      memcpy(dest + (py - sliceY) * screenWidth, source + (py - dy) * DigitSheetWidth, size);
    }
  }
}


// -------------------------------------------------------------------------
// Memory transfer to DMA controller
// -------------------------------------------------------------------------

// initiates memory forwarding to the DMA controller....
void customDrawBuffer(uint8_t x, uint8_t y, uint16_t* buffer, uint8_t w, uint8_t h) {
  drawPending = true;
  gb.tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
  SPI.beginTransaction(Gamebuino_Meta::tftSPISettings);
  gb.tft.dataMode();
  gb.tft.sendBuffer(buffer, w * h);
}

// waits for the memory transfer to be completed
// and close the transaction with the DMA controller
void waitForPreviousDraw() {
  if (drawPending) {
    Gamebuino_Meta::wait_for_transfers_done();
    gb.tft.idleMode();
    SPI.endTransaction();
    drawPending = false;
  }
}


void drawBackground(uint8_t background, const Sprite spriteToDisplay, uint8_t spriteX, uint8_t spriteY, boolean displaySprite)
{
  for (uint8_t sliceIndex = 0; sliceIndex < slices; sliceIndex++)
  {
    // buffers are switched according to the parity of sliceIndex
    uint16_t * buffer = (sliceIndex % 2 == 0) ? buffer1 : buffer2;

    // the top border of the current slice is calculated
    uint8_t sliceY = sliceIndex * sliceHeight;

    // starts by drawing the background
    picRead(buffer, background, 0, sliceY, screenWidth * sliceHeight);

    // and finally draws the sprite if needed
    if (displaySprite == true) drawText(spriteToDisplay, sliceY, buffer, spriteX, spriteY);

    // then we make sure that the sending of the previous buffer
    // to the DMA controller has taken place
    if (sliceIndex != 0)
      waitForPreviousDraw();

    // after which we can then send the current buffer
    customDrawBuffer(0, sliceY, buffer, screenWidth, sliceHeight);
  }

  // always wait until the DMA transfer is completed
  // for the last slice before entering the next cycle
  waitForPreviousDraw();
}

#endif
