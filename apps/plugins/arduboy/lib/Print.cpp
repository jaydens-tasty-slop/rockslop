#include "Print.h"
#include <string.h>

size_t Print::write(const uint8_t *buf, size_t n)
{
  size_t i = 0;
  while (i < n && write(buf[i])) i++;
  return i;
}

size_t Print::write(const char *s)
{
  return s ? write((const uint8_t *)s, strlen(s)) : 0;
}

size_t Print::printNumber(unsigned long n, uint8_t base)
{
  char buf[33];
  char *p = &buf[sizeof(buf) - 1];
  *p = '\0';
  if (base < 2) base = 10;
  do {
    unsigned long m = n;
    n /= base;
    char c = m - base * n;
    *--p = c < 10 ? c + '0' : c + 'A' - 10;
  } while (n);
  return write(p);
}

size_t Print::print(long v, int base)
{
  if (base == 10 && v < 0) {
    return write('-') + printNumber(-(unsigned long)v, 10);
  }
  return printNumber(v, base);
}

/* Fixed-point only: the plugin ABI is soft-float and avoids float math. */
size_t Print::print(double v, int digits)
{
  size_t n = 0;
  if (v < 0) {
    n += write('-');
    v = -v;
  }
  unsigned long whole = (unsigned long)v;
  n += printNumber(whole, 10);
  if (digits > 0) {
    n += write('.');
    v -= whole;
    while (digits-- > 0) {
      v *= 10;
      unsigned d = (unsigned)v;
      n += write((uint8_t)('0' + d));
      v -= d;
    }
  }
  return n;
}
