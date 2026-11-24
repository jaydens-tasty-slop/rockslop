#ifndef EEPROM_UTILS_H
#define EEPROM_UTILS_H

#include <EEPROM.h>

#define EEPROM_START      (EEPROM_STORAGE_SPACE_START + 72)

void EEPROM_saveScore(uint8_t w, uint8_t l) {
  EEPROM.update(EEPROM_START+2, w);
  EEPROM.update(EEPROM_START+3, l);
}

uint8_t EEPROM_getWins() {
  return EEPROM.read(EEPROM_START+2);
}

uint8_t EEPROM_getLosses() {
  return EEPROM.read(EEPROM_START+3);
}

void EEPROM_init() {
  uint8_t h1 = EEPROM.read(EEPROM_START+0);
  uint8_t h2 = EEPROM.read(EEPROM_START+1);

  if (h1 != 'H' || h2 != 'M') {
    EEPROM.update(EEPROM_START+0, 'H');
    EEPROM.update(EEPROM_START+1, 'M');
    EEPROM_saveScore(0, 0);
  }
}

#endif
