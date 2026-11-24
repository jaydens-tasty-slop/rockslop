/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Classic Gamebuino (<Gamebuino.h>, 84x48) names over the META layer, for the
 * Classic sketches (gbjezzball, gbbangbang, gbsolitaire): button names, LCD
 * size, EEPROM on the layer's save file, malloc/free on the layer's heap. */
#ifndef GB_CLASSIC_H
#define GB_CLASSIC_H
#include <Gamebuino-Meta.h>
#include <math.h>

#define LCDWIDTH 84
#define LCDHEIGHT 48
#define BTN_UP BUTTON_UP
#define BTN_DOWN BUTTON_DOWN
#define BTN_LEFT BUTTON_LEFT
#define BTN_RIGHT BUTTON_RIGHT
#define BTN_A BUTTON_A
#define BTN_B BUTTON_B
#define BTN_C BUTTON_MENU
typedef uint8_t byte;

struct EepromClass {
    template <typename T> void get(int i, T &v) {
        gb_save_load(256);
        memcpy(&v, gb_save_data + i, sizeof(T));
    }
    uint8_t read(int i) { gb_save_load(256); return gb_save_data[i]; }
    void update(int i, uint8_t v) {
        gb_save_load(256);
        if (gb_save_data[i] != v) {
            gb_save_data[i] = v;
            gb_save_dirty();
        }
    }
    template <typename T> void put(int i, const T &v) {
        gb_save_load(256);
        memcpy(gb_save_data + i, &v, sizeof(T));
        gb_save_dirty();
    }
};
static EepromClass EEPROM;

/* new/delete already use this heap (gb.cpp). */
#define malloc gb_alloc
#define free gb_release
#endif
