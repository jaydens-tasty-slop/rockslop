#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <Arduino.h>
#include "globals.h"

#define POWER_UP_WIDTH                          11
#define POWER_UP_HEIGHT                         11
#define AMOUNT_OF_POWER_UPS                     24

#define POWER_UP_ONE                            0
#define POWER_UP_TWO                            1
#define POWER_UP_THREE                          2
#define POWER_UP_FOUR                           3
#define POWER_UP_FIVE                           4
#define POWER_UP_SIX                            5
#define POWER_UP_SEVEN                          6
#define POWER_UP_EIGHT                          7
#define POWER_UP_NINE                           8
#define POWER_UP_TEN                            9
#define POWER_UP_ELEVEN                         10
#define POWER_UP_TWELVE                         11
#define POWER_UP_THIRTEEN                       12
#define POWER_UP_FOURTEEN                       13
#define POWER_UP_FIFTEEN                        14
#define POWER_UP_SIXTEEN                        15
#define POWER_UP_SEVENTEEN                      16
#define POWER_UP_EIGHTEEN                       17
#define POWER_UP_NINETEEN                       18
#define POWER_UP_TWENTY                         19
#define POWER_UP_TWENTY_ONE                     20
#define POWER_UP_TWENTY_TWO                     21
#define POWER_UP_TWENTY_THREE                   22
#define POWER_UP_TWENTY_FOUR                    23

// test
PROGMEM const uint8_t powerUpData[] = {
// powerUp data length
0x07,
// type (0-23), road block (1 block = 32 pixel), x (0-31), y (0-63)
0x00, 0x05, 0x01, 0x1a,
0x01, 0x09, 0x02, 0x1b,
0x02, 0x0d, 0x03, 0x1c,
0x03, 0x11, 0x03, 0x10,
0x04, 0x15, 0x03, 0x12,
0x05, 0x17, 0x03, 0x1c,
0x06, 0x1a, 0x03, 0x1c,
0x07, 0x1e, 0x03, 0x1c,
};

struct PowerUpObject
{
  public:
    int x;
    int y;
    byte type;
};

struct PowerUp
{
  public:
    uint8_t len;
    PowerUpObject obj[16];
};

PowerUp powerUp;

void setPowerUp()
{
  powerUp.len = pgm_read_byte_near(powerUpData);

  for (uint8_t len = 0; len < powerUp.len; len++)
  {
    uint8_t pos = (len * 4) + 1;
    powerUp.obj[len].type = pgm_read_byte_near(powerUpData + pos);
    powerUp.obj[len].x = (pgm_read_byte_near(powerUpData + pos + 1) * 32) + pgm_read_byte_near(powerUpData + pos + 2);
    powerUp.obj[len].y = pgm_read_byte_near(powerUpData + pos + 3);
  }
}

void drawPowerUp()
{
  for (uint8_t len = 0; len < powerUp.len; len++)
  {
    sprites.drawPlusMask(powerUp.obj[len].x - road.x, powerUp.obj[len].y, powerUpsBorder_plus_mask, 0);
    sprites.drawSelfMasked(powerUp.obj[len].x + 3 - road.x, powerUp.obj[len].y + 3, powerUps, powerUp.obj[len].type);
  }
}

#endif
