#pragma once
/* YP3 port: upstream's EEPROM save is commented out (only the "saved" flag
 * byte is used), so a RAM byte stands in for it; its debug printfs go. */
#include <stdint.h>

struct RogueEeprom {
    uint8_t flag;
    uint8_t read(int) const { return flag; }
    void update(int, uint8_t v) { flag = v; }
};
static RogueEeprom EEPROM;
#define printf(...) ((void)0)
