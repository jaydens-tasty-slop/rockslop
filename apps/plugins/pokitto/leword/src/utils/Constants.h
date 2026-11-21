#pragma once

#define SOUNDS
#define COOKIE_INITIALISED 47

#define _USE_BRINE
/* TEASE PRUNE DRONE CRIME BLINK and BRIEF or BRINE */

namespace Constants {

    constexpr uint8_t Guess_Left = 25;
    constexpr uint8_t Guess_Spacing = 12;

    constexpr uint8_t Key_Map[3][10] = { 
        { 16, 22,  4, 17, 19, 24, 20,  8, 14,  15, },
        {  0, 18,  3,  5,  6,  7,  9, 10, 11, 255},
        { 26, 25, 23,  2, 21,  1, 13, 12, 27, 255  }
        }; 

    constexpr int8_t Scroll_Limits[] = { 0, 0, 0, 0, 18, 5 };
    constexpr int8_t shake[] = { 0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1, -2, -1, };
    constexpr uint16_t Delay_Low = 400;
    constexpr uint16_t Delay_High = 1500;
    constexpr uint16_t Key_Flash = 32;
    constexpr uint8_t NumberOfLetters = 14;

    constexpr uint8_t Black = 0;
    constexpr uint8_t White = 7;
    constexpr uint8_t Orange = 0xFC;
    constexpr uint8_t Green = 0x35;
    constexpr uint8_t DarkGrey = 2;
    constexpr uint8_t LightGrey = 5;
    constexpr uint8_t Transparent = 255;
  

};

