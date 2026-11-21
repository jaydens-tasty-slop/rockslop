/* Rockbox replacement for ArduboyCoreRem (the hardware layer of Ard-Drivin's
 * private Arduboy2 variant). Same interface; OLED/AVR code is gone and the
 * frame goes to the Rockbox display through ab_glue.c. */
#ifndef ARDUBOY_CORE_H
#define ARDUBOY_CORE_H

#include <Arduino.h>
#include <SPI.h>
#include <limits.h>

const uint8_t PROGMEM yMask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

#define ARDUBOY_10
#define RGB_ON LOW
#define RGB_OFF HIGH

#define RED_LED 10
#define GREEN_LED 11
#define BLUE_LED 9
#define TX_LED 30
#define RX_LED 17

#define LEFT_BUTTON AB_B_LEFT
#define RIGHT_BUTTON AB_B_RIGHT
#define UP_BUTTON AB_B_UP
#define DOWN_BUTTON AB_B_DOWN
#define A_BUTTON AB_B_A
#define B_BUTTON AB_B_B

#define PIN_SPEAKER_1 5
#define PIN_SPEAKER_2 13

#define OLED_PIXELS_INVERTED 0xA7
#define OLED_PIXELS_NORMAL 0xA6
#define OLED_ALL_PIXELS_ON 0xA5
#define OLED_PIXELS_FROM_RAM 0xA4
#define OLED_VERTICAL_FLIPPED 0xC0
#define OLED_VERTICAL_NORMAL 0xC8
#define OLED_HORIZ_FLIPPED 0xA0
#define OLED_HORIZ_NORMAL 0xA1

#define WIDTH 128
#define HEIGHT 64

class ArduboyCoreRem
{
  public:
    ArduboyCoreRem() {}

    void static idle() { ab_idle(); }
    void static LCDDataMode() {}
    void static LCDCommandMode() {}
    uint8_t static width() { return WIDTH; }
    uint8_t static height() { return HEIGHT; }
    uint8_t static buttonsState() { return ab_buttons(); }
    void static paint8Pixels(uint8_t) {}
    void static paintScreen(const uint8_t *image);
    void static paintScreen(uint8_t image[]);
    void static blank();
    void static invert(bool) {}
    void static allPixelsOn(bool) {}
    void static flipVertical(bool) {}
    void static flipHorizontal(bool) {}
    void static sendLCDCommand(uint8_t) {}
    void static setRGBled(uint8_t red, uint8_t green, uint8_t blue) { ab_set_led(red, green, blue); }
    void static digitalWriteRGB(uint8_t, uint8_t, uint8_t) {}
    void static boot() {}

    static byte flicker;
};

#endif
