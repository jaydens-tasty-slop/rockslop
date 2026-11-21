/* SPDX-License-Identifier: GPL-2.0-or-later */
/* EEPROM emulated by a per-game file; see ab_glue.c. */
#ifndef EEPROM_H
#define EEPROM_H
#include <stdint.h>
#include "ab_glue.h"

struct EERef
{
  EERef(int i) : index(i) {}
  operator uint8_t() const { return ab_eeprom[index]; }
  EERef &operator=(uint8_t v)
  {
    if (ab_eeprom[index] != v) {
      ab_eeprom[index] = v;
      ab_eeprom_dirty();
    }
    return *this;
  }
  EERef &operator=(const EERef &r) { return *this = (uint8_t)r; }
  EERef &operator+=(uint8_t v) { return *this = (uint8_t)*this + v; }
  EERef &operator-=(uint8_t v) { return *this = (uint8_t)*this - v; }
  EERef &operator++() { return *this += 1; }
  EERef &operator--() { return *this -= 1; }
  int index;
};

struct EEPROMClass
{
  EERef operator[](int i) { return EERef(i & (AB_EEPROM_SIZE - 1)); }
  uint8_t read(int i) { return (uint8_t)EERef(i & (AB_EEPROM_SIZE - 1)); }
  void write(int i, uint8_t v) { EERef(i & (AB_EEPROM_SIZE - 1)) = v; }
  void update(int i, uint8_t v) { write(i, v); }
  bool commit() { ab_eeprom_flush(); return true; }
  uint16_t length() { return AB_EEPROM_SIZE; }
  template <typename T> T &get(int idx, T &t)
  {
    uint8_t *p = (uint8_t *)&t;
    for (unsigned i = 0; i < sizeof(T); i++) *p++ = read(idx + i);
    return t;
  }
  template <typename T> const T &put(int idx, const T &t)
  {
    const uint8_t *p = (const uint8_t *)&t;
    for (unsigned i = 0; i < sizeof(T); i++) write(idx + i, *p++);
    return t;
  }
};
extern EEPROMClass EEPROM;

/* <avr/eeprom.h> functions; "pointers" are EEPROM addresses. */
static inline uint8_t eeprom_read_byte(const uint8_t *p)
{
  return ab_eeprom[(uintptr_t)p & (AB_EEPROM_SIZE - 1)];
}
static inline void eeprom_update_byte(uint8_t *p, uint8_t v)
{
  EEPROM.write((int)(uintptr_t)p, v);
}
static inline void eeprom_write_byte(uint8_t *p, uint8_t v)
{
  EEPROM.write((int)(uintptr_t)p, v);
}
static inline uint16_t eeprom_read_word(const uint16_t *p)
{
  return eeprom_read_byte((const uint8_t *)p) |
         ((uint16_t)eeprom_read_byte((const uint8_t *)p + 1) << 8);
}
static inline void eeprom_update_word(uint16_t *p, uint16_t v)
{
  eeprom_update_byte((uint8_t *)p, v & 0xff);
  eeprom_update_byte((uint8_t *)p + 1, v >> 8);
}
static inline void eeprom_write_word(uint16_t *p, uint16_t v)
{
  eeprom_update_word(p, v);
}
static inline void eeprom_busy_wait(void) {}
static inline uint32_t eeprom_read_dword(const uint32_t *p)
{
  return eeprom_read_word((const uint16_t *)p) |
         ((uint32_t)eeprom_read_word((const uint16_t *)p + 1) << 16);
}
static inline void eeprom_update_dword(uint32_t *p, uint32_t v)
{
  eeprom_update_word((uint16_t *)p, v & 0xffff);
  eeprom_update_word((uint16_t *)p + 1, v >> 16);
}
static inline void eeprom_write_dword(uint32_t *p, uint32_t v)
{
  eeprom_update_dword(p, v);
}
static inline void eeprom_read_block(void *dst, const void *src, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  for (size_t i = 0; i < n; i++) d[i] = eeprom_read_byte((const uint8_t *)src + i);
}
static inline void eeprom_update_block(const void *src, void *dst, size_t n)
{
  const uint8_t *s = (const uint8_t *)src;
  for (size_t i = 0; i < n; i++) eeprom_update_byte((uint8_t *)dst + i, s[i]);
}
static inline void eeprom_write_block(const void *src, void *dst, size_t n)
{
  eeprom_update_block(src, dst, n);
}
#endif
