/* Arduboy2Core for Rockbox. The AVR/OLED hardware layer of Arduboy2 is
 * replaced by ab_glue.c; the public interface is kept. Based on Arduboy2
 * (BSD-3-Clause, see LICENSE.Arduboy2). */
#ifndef ARDUBOY2_CORE_H
#define ARDUBOY2_CORE_H
#include <Arduino.h>

#define ARDUBOY_10
#define RGB_ON LOW
#define RGB_OFF HIGH
#define RED_LED 10
#define GREEN_LED 11
#define BLUE_LED 9

#define LEFT_BUTTON AB_B_LEFT
#define RIGHT_BUTTON AB_B_RIGHT
#define UP_BUTTON AB_B_UP
#define DOWN_BUTTON AB_B_DOWN
#define A_BUTTON AB_B_A
#define B_BUTTON AB_B_B

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
#define COLUMN_ADDRESS_END (WIDTH - 1) & 127
#define PAGE_ADDRESS_END ((HEIGHT/8)-1) & 7

#define ARDUBOY_NO_USB

class Arduboy2NoUSB {};

class Arduboy2Core : public Arduboy2NoUSB
{
  friend class Arduboy2Ex;

  public:
    Arduboy2Core() {}
    static void idle();
    static void LCDDataMode() {}
    static void LCDCommandMode() {}
    static void SPItransfer(uint8_t) {}
    static uint8_t SPItransferAndRead(uint8_t) { return 0; }
    static void displayOff() {}
    static void displayOn() {}
    static constexpr uint8_t width() { return WIDTH; }
    static constexpr uint8_t height() { return HEIGHT; }
    static uint8_t buttonsState();
    static void paint8Pixels(uint8_t) {}
    static void paintScreen(const uint8_t *image);
    static void paintScreen(uint8_t image[], bool clear = false);
    static void blank();
    static void invert(bool inverse);
    static void allPixelsOn(bool on);
    static void flipVertical(bool) {}
    static void flipHorizontal(bool) {}
    static void sendLCDCommand(uint8_t command);
    static void setRGBled(uint8_t red, uint8_t green, uint8_t blue);
    static void setRGBled(uint8_t, uint8_t) {}
    static void freeRGBled() {}
    static void digitalWriteRGB(uint8_t red, uint8_t green, uint8_t blue);
    static void digitalWriteRGB(uint8_t, uint8_t) {}
    static void boot();
    static void safeMode() {}
    static unsigned long generateRandomSeed();
    static void delayShort(uint16_t ms) { delay(ms); }
    static void exitToBootloader() {}

  protected:
    static bool inverted;
    static bool allOn;
};
#endif
