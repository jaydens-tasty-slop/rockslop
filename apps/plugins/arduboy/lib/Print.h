/* Print without String: the subset the Arduboy2 text API needs. */
#ifndef PRINT_H
#define PRINT_H
#include <stdint.h>
#include <stddef.h>

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper *>(s))

class Print
{
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *buf, size_t n);
  size_t write(const char *s);
  size_t write(const char *buf, size_t n) { return write((const uint8_t *)buf, n); }

  size_t print(const __FlashStringHelper *s) { return write((const char *)s); }
  size_t print(const char *s) { return write(s); }
  size_t print(const class String &s);
  size_t println(const class String &s);
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(unsigned char v, int base = DEC) { return printNumber(v, base); }
  size_t print(int v, int base = DEC) { return print((long)v, base); }
  size_t print(unsigned int v, int base = DEC) { return printNumber(v, base); }
  size_t print(long v, int base = DEC);
  size_t print(unsigned long v, int base = DEC) { return printNumber(v, base); }
  size_t print(double v, int digits = 2);
  size_t println(void) { return write("\r\n"); }
  size_t println(const __FlashStringHelper *s) { return print(s) + println(); }
  size_t println(const char *s) { return print(s) + println(); }
  size_t println(char c) { return print(c) + println(); }
  size_t println(unsigned char v, int base = DEC) { return print(v, base) + println(); }
  size_t println(int v, int base = DEC) { return print(v, base) + println(); }
  size_t println(unsigned int v, int base = DEC) { return print(v, base) + println(); }
  size_t println(long v, int base = DEC) { return print(v, base) + println(); }
  size_t println(unsigned long v, int base = DEC) { return print(v, base) + println(); }
  size_t println(double v, int digits = 2) { return print(v, digits) + println(); }

 private:
  size_t printNumber(unsigned long n, uint8_t base);
};
#include "WString.h"
#endif
