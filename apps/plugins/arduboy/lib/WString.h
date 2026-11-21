/* SPDX-License-Identifier: GPL-2.0-or-later */
/* A small Arduino String for sketches that use it for short messages: a fixed
 * buffer (longer text is cut off), no heap. Only the common operations. */
#ifndef AB_WSTRING_H
#define AB_WSTRING_H
#include "Print.h"

#ifndef AB_STRING_CAP
#define AB_STRING_CAP 119
#endif

class String
{
 public:
  String() { b[0] = 0; n = 0; }
  String(const char *s) { b[0] = 0; n = 0; if (s) add(s); }
  String(const __FlashStringHelper *s) { b[0] = 0; n = 0; if (s) add((const char *)s); }
  String(const String &o) { copyFrom(o); }
  String(char c) { b[0] = 0; n = 0; add(c); }
  String(unsigned char v, unsigned char base = 10) { init((unsigned long)v, base, false); }
  String(int v, unsigned char base = 10) { init((long)v, base); }
  String(unsigned int v, unsigned char base = 10) { init((unsigned long)v, base, false); }
  String(long v, unsigned char base = 10) { init(v, base); }
  String(unsigned long v, unsigned char base = 10) { init(v, base, false); }

  String &operator=(const String &o) { if (this != &o) copyFrom(o); return *this; }
  String &operator=(const char *s) { b[0] = 0; n = 0; if (s) add(s); return *this; }
  String &operator=(const __FlashStringHelper *s) { b[0] = 0; n = 0; if (s) add((const char *)s); return *this; }

  String &operator+=(const String &o) { add(o.b); return *this; }
  String &operator+=(const char *s) { if (s) add(s); return *this; }
  String &operator+=(const __FlashStringHelper *s) { if (s) add((const char *)s); return *this; }
  String &operator+=(char c) { add(c); return *this; }
  String &operator+=(int v) { return *this += String(v); }
  String &operator+=(unsigned int v) { return *this += String(v); }
  String &operator+=(long v) { return *this += String(v); }
  String &operator+=(unsigned long v) { return *this += String(v); }
  String &operator+=(unsigned char v) { return *this += String(v); }
  bool concat(const String &o) { *this += o; return true; }
  bool concat(const char *s) { *this += s; return true; }
  bool concat(char c) { *this += c; return true; }

  unsigned int length() const { return n; }
  const char *c_str() const { return b; }
  int indexOf(char c, unsigned int from = 0) const {
    for (unsigned int i = from; i < n; i++) if (b[i] == c) return i;
    return -1;
  }
  String substring(unsigned int from, unsigned int to) const {
    String r;
    if (to > n) to = n;
    for (unsigned int i = from; i < to; i++) r.add(b[i]);
    return r;
  }
  String substring(unsigned int from) const { return substring(from, n); }
  char charAt(unsigned int i) const { return i < n ? b[i] : 0; }
  char operator[](unsigned int i) const { return charAt(i); }
  bool operator==(const String &o) const { return cmp(o.b) == 0; }
  bool operator==(const char *s) const { return cmp(s) == 0; }
  bool operator!=(const String &o) const { return cmp(o.b) != 0; }
  bool operator!=(const char *s) const { return cmp(s) != 0; }
  void remove(unsigned int idx) { if (idx < n) { n = idx; b[n] = 0; } }

 private:
  char b[AB_STRING_CAP + 1];
  unsigned char n;
  void add(char c) { if (n < AB_STRING_CAP) { b[n++] = c; b[n] = 0; } }
  void add(const char *s) { while (*s && n < AB_STRING_CAP) b[n++] = *s++; b[n] = 0; }
  void copyFrom(const String &o) { n = o.n; for (unsigned i = 0; i <= n; i++) b[i] = o.b[i]; }
  int cmp(const char *s) const { const char *p = b; while (*p && *p == *s) { p++; s++; } return (unsigned char)*p - (unsigned char)*s; }
  void init(unsigned long v, unsigned char base, bool isSigned = false, bool neg = false)
  {
    char t[34]; int k = 0;
    if (base < 2) base = 10;
    if (v == 0) t[k++] = '0';
    while (v) { unsigned d = v % base; t[k++] = d < 10 ? '0' + d : 'A' + d - 10; v /= base; }
    b[0] = 0; n = 0;
    if (neg) add('-');
    while (k) add(t[--k]);
    (void)isSigned;
  }
  void init(long v, unsigned char base) { init(v < 0 ? (unsigned long)(-v) : (unsigned long)v, base, true, v < 0); }
};

inline String operator+(const String &a, const String &b) { String r(a); r += b; return r; }
inline String operator+(const String &a, const char *b) { String r(a); r += b; return r; }
inline String operator+(const char *a, const String &b) { String r(a); r += b; return r; }
inline String operator+(const String &a, const __FlashStringHelper *b) { String r(a); r += b; return r; }
inline String operator+(const __FlashStringHelper *a, const String &b) { String r(a); r += b; return r; }
inline String operator+(const String &a, char b) { String r(a); r += b; return r; }
inline String operator+(const String &a, int b) { String r(a); r += b; return r; }
inline String operator+(const String &a, unsigned int b) { String r(a); r += b; return r; }
inline String operator+(const String &a, long b) { String r(a); r += b; return r; }
inline String operator+(const String &a, unsigned long b) { String r(a); r += b; return r; }

inline size_t Print::print(const String &s) { return write(s.c_str()); }
inline size_t Print::println(const String &s) { return print(s) + println(); }
#endif
