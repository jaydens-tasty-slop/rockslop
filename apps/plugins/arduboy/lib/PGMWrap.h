#ifndef PGMWRAP_H
#define PGMWRAP_H
/* PGMWrap (Arduino library): on the AVR the *_p types read from program
 * memory transparently. Flash is plain memory here, so they are the types. */
#include <Arduino.h>

typedef bool     bool_p;
typedef char     char_p;
typedef uint8_t  uint8_p;
typedef int8_t   int8_p;
typedef uint16_t uint16_p;
typedef int16_t  int16_p;
typedef uint32_t uint32_p;
typedef int32_t  int32_p;
typedef float    float_p;
typedef double   double_p;
#endif
