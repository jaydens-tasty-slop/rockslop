#pragma once

#define _USE_BRINE
/* TEASE PRUNE DRONE CRIME BLINK and BRIEF or BRINE */

namespace Constants {

    constexpr uint8_t guess_Left = 32;
    constexpr uint8_t guess_Spacing = 13;

    constexpr uint8_t key_Map[3][10] = { 
        { 16, 22,  4, 17, 19, 24, 20,  8, 14,  15, },
        {  0, 18,  3,  5,  6,  7,  9, 10, 11, 255},
        { 26, 25, 23,  2, 21,  1, 13, 12, 27, 255  }
        }; 

    constexpr int8_t scroll_Limits[] = { 0, 0, 18, 5, -6, -20 };

    constexpr int8_t shake[] = { 0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1, -2, -1, };

    constexpr uint16_t EEPROM_Char1 = 78;
    constexpr uint16_t EEPROM_Char2 = EEPROM_Char1 + 1;
    constexpr uint16_t EEPROM_Mode = EEPROM_Char2 + 1;
    constexpr uint16_t EEPROM_Games_Won_EN = EEPROM_Mode + 1;
    constexpr uint16_t EEPROM_Games_Played_EN = EEPROM_Games_Won_EN + 2;
    constexpr uint16_t EEPROM_Current_Streak_EN = EEPROM_Games_Played_EN + 2;
    constexpr uint16_t EEPROM_Max_Streak_EN = EEPROM_Current_Streak_EN + 2;
    constexpr uint16_t EEPROM_Games_Won_FR = EEPROM_Max_Streak_EN + 2;
    constexpr uint16_t EEPROM_Games_Played_FR = EEPROM_Games_Won_FR + 2;
    constexpr uint16_t EEPROM_Current_Streak_FR = EEPROM_Games_Played_FR + 2;
    constexpr uint16_t EEPROM_Max_Streak_FR = EEPROM_Current_Streak_FR + 2;
    constexpr uint16_t EEPROM_Distribution_EN = EEPROM_Max_Streak_FR + 2;
    constexpr uint16_t EEPROM_Distribution_FR = EEPROM_Distribution_EN + 12;


    constexpr uint16_t Delay_Low = 400;
    constexpr uint16_t Delay_High = 1500;

};

