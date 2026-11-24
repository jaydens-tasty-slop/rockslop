#pragma once

#include <Gamebuino-Meta.h>

// The YP3 has no potentiometers: the pins are stand-ins and every reading is 0.
#define INPUT 0
#define A4 4
#define A5 5
static inline void pinMode(uint8_t, uint8_t) {}
static inline uint16_t analogRead(uint8_t) { return 0; }
