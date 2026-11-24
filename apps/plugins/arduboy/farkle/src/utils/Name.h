#pragma once

#include <Arduboy2.h>
#include "Enums.h"

class Name {

  public: 

    Name();
  

    // Properties ..

    uint8_t getCharIndex();
    char getChar(uint8_t idx);
    void setCharIndex(uint8_t val);
    void setChar(uint8_t idx, uint8_t val);
    void setChars(char val[NAME_LENGTH]);


    // Methods ..

    void incCharIndex();
    void decCharIndex();
    void incChar(uint8_t idx);
    void decChar(uint8_t idx);

    void clear();
    void clear(uint16_t startingLocation);
    void retrieve(uint16_t startingLocation);
    void save(uint16_t startingLocation);
    char* getString();
    uint8_t getIcon();

    void clearIconsInUse();
    void addIconsInUse(uint8_t icon);
    void setStartLoc(uint16_t eeprom_start_loc);

  private:
   
    uint8_t _charIndex;
    char _chars[NAME_LENGTH];
    uint16_t eeprom_start_loc;
    uint8_t iconsInUse[3];
    uint8_t iconsIndex;

};
