#include "ArduboyCoreRem.h"

byte ArduboyCoreRem::flicker = 0;

void ArduboyCoreRem::paintScreen(const uint8_t *image)
{
  ab_present(image, 0);
}

/* Paints the buffer, then clears it to the flicker-grey pattern (a checkerboard
 * that alternates every frame), as the original hardware routine did. The
 * Rockbox glue blends successive frames, which turns the flicker into grey. */
void ArduboyCoreRem::paintScreen(uint8_t image[])
{
  ab_present(image, 0);
  uint8_t alternate = (flicker & 1) ? 0x55 : 0xAA;
  for (int i = 0; i < (HEIGHT * WIDTH) / 8; i++) {
    image[i] = alternate;
    alternate = ~alternate;
  }
}

void ArduboyCoreRem::blank()
{
  static const uint8_t zero[(HEIGHT * WIDTH) / 8] = {0};
  ab_present(zero, 0);
}
