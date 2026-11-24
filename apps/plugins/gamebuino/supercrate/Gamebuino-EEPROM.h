/* The Gamebuino EEPROM emulation (256 bytes) on the layer's save file. */
#ifndef SUPERCRATE_EEPROM_H
#define SUPERCRATE_EEPROM_H
#include <Gamebuino-Meta.h>

struct EepromClass {
    void begin(size_t size) { gb_save_load(size); }
    uint8_t read(int i) { return gb_save_data[i]; }
    void write(int i, uint8_t v) {
        if (gb_save_data[i] != v) {
            gb_save_data[i] = v;
            gb_save_dirty();
        }
    }
};
static EepromClass EEPROM;
#endif
