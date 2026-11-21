/* Rockbox replacement for the Arduboy v1 "ArduboyCore" hardware class that
 * Ardulem bundles (Arduboy library, BSD-3-Clause). The interface that
 * CustomArduboy uses is kept; the OLED/AVR code is gone. */
#ifndef ArduboyCore_h
#define ArduboyCore_h

#include <Arduino.h>
#include <SPI.h>
#include <limits.h>

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

#define WIDTH 128
#define HEIGHT 64

#define INVERT 2
#define WHITE 1
#define BLACK 0

class ArduboyCore
{
public:
    ArduboyCore() {}

    static void idle() { ab_idle(); }
    static void LCDDataMode() {}
    static void LCDCommandMode() {}
    static uint8_t width() { return WIDTH; }
    static uint8_t height() { return HEIGHT; }
    static uint8_t getInput() { return ab_buttons(); }
    static uint8_t buttonsState() { return ab_buttons(); }
    static void paint8Pixels(uint8_t) {}
    static void paintScreen(const unsigned char *image) { ab_present(image, inverted); }
    static void paintScreen(unsigned char image[]) { ab_present(image, inverted); }
    static void blank();
    static void invert(boolean inverse) { inverted = inverse; }
    static void allPixelsOn(boolean) {}
    static void flipVertical(boolean) {}
    static void flipHorizontal(boolean) {}
    static void sendLCDCommand(uint8_t) {}
    void setRGBled(uint8_t red, uint8_t green, uint8_t blue) { ab_set_led(red, green, blue); }

protected:
    static void boot() {}

private:
    static bool inverted;
};

#endif
