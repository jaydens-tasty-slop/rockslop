/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Minimal Arduino core for the Rockbox Arduboy layer. */
#ifndef ARDUINO_H
#define ARDUINO_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ab_glue.h"
#include <avr/pgmspace.h>

#define ARDUINO 10819
#define F_CPU 16000000UL
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define _BV(b) (1 << (b))
#ifndef bit
#define bit(b) (1UL << (b))
#endif
#define PI 3.1415926535897932384626433832795
#define TXLED0 ((void)0)
#define TXLED1 ((void)0)
#define RXLED0 ((void)0)
#define RXLED1 ((void)0)

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef abs
#define abs(x) ((x) > 0 ? (x) : -(x))
#endif
#define power_timer0_disable() ((void)0)
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define sq(x) ((x) * (x))
#define radians(d) ((d) * 0.017453292519943295)
#define degrees(r) ((r) * 57.29577951308232)
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define bitRead(v, b) (((v) >> (b)) & 1)
#define bitSet(v, b) ((v) |= (1UL << (b)))
#define bitClear(v, b) ((v) &= ~(1UL << (b)))
#define bitToggle(v, b) ((v) ^= (1UL << (b)))
#define bitWrite(v, b, x) ((x) ? bitSet(v, b) : bitClear(v, b))
#define interrupts() ((void)0)
#define noInterrupts() ((void)0)
#define sei() ((void)0)
#define cli() ((void)0)

typedef uint8_t byte;
typedef bool boolean;
typedef uint16_t word;

static inline unsigned long millis(void) { return ab_millis(); }
static inline unsigned long micros(void) { return ab_millis() * 1000UL; }
static inline void delay(unsigned long ms) { ab_delay(ms); }
static inline void delayMicroseconds(unsigned int us) { if (us >= 1000) ab_delay(us / 1000); }
static inline void randomSeed(unsigned long s) { ab_srand(s); }
static inline long random(long hi) { return hi > 0 ? (long)(ab_rand() % (unsigned long)hi) : 0; }
static inline long random(long lo, long hi) { return hi > lo ? lo + random(hi - lo) : lo; }
static inline long map(long x, long a, long b, long c, long d)
{
  return (x - a) * (d - c) / (b - a) + c;
}
/* AVR libc <stdlib.h> number to string (negative only in base 10) */
static inline char *ultoa(unsigned long v, char *s, int base)
{
  char tmp[33];
  int n = 0;
  do {
    unsigned d = v % base;
    tmp[n++] = d < 10 ? '0' + d : 'a' + d - 10;
    v /= base;
  } while (v);
  char *p = s;
  while (n)
    *p++ = tmp[--n];
  *p = 0;
  return s;
}
static inline char *ltoa(long v, char *s, int base)
{
  if (v < 0 && base == 10) {
    *s = '-';
    ultoa(-(unsigned long)v, s + 1, base);
    return s;
  }
  return ultoa((unsigned long)v, s, base);
}
static inline char *itoa(int v, char *s, int base) { return ltoa(v, s, base); }
static inline char *utoa(unsigned v, char *s, int base) { return ultoa(v, s, base); }

static inline void pinMode(uint8_t, uint8_t) {}
static inline void digitalWrite(uint8_t, uint8_t) {}
static inline int digitalRead(uint8_t) { return HIGH; }
static inline int analogRead(uint8_t) { return 0; }

/* AVR timer 0 counter, only ever used as a source of jitter */
#define TCNT0 ((uint8_t)ab_rand())

/* Setup/loop entry points supplied by the game (the sketch). */
void setup(void);
void loop(void);

#include "Print.h"

class SerialClass : public Print
{
 public:
  void begin(unsigned long) {}
  void end() {}
  operator bool() const { return false; }
  int available() { return 0; }
  int read() { return -1; }
  void flush() {}
  virtual size_t write(uint8_t) { return 1; }
  using Print::write;
};
extern SerialClass Serial;
#endif
