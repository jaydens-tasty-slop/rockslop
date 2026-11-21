/* SPDX-License-Identifier: GPL-2.0-or-later */
/* AVR PROGMEM is plain memory here: all reads are ordinary loads. */
#ifndef AVR_PGMSPACE_H
#define AVR_PGMSPACE_H
#include <stdint.h>
#include <stddef.h>
#include "ab_glue.h"

#define PROGMEM
#define PGM_P const char *
#define PGM_VOID_P const void *
#define PSTR(s) (s)
#define pgm_read_byte(a) (*(const uint8_t *)(a))
#define pgm_read_word(a) (*(const uint16_t *)(a))
#define pgm_read_dword(a) (*(const uint32_t *)(a))
#define pgm_read_ptr(a) (*(void *const *)(a))
#ifdef __cplusplus
/* pgm_read_word on a table of pointers is a 16-bit pointer read on AVR; here
 * the pointer is 32 bits, so reading through a pointer-to-pointer returns the
 * pointer itself (other tables keep the 16-bit read). */
template <class T> static inline T *ab_pgm_word(T *const *p) { return *p; }
static inline uint16_t ab_pgm_word(const volatile void *p) { return *(const volatile uint16_t *)p; }
#undef pgm_read_word
#define pgm_read_word(a) ab_pgm_word(a)
#endif
#define pgm_read_float(a) (*(const float *)(a))
#define pgm_read_byte_near pgm_read_byte
#define pgm_read_word_near pgm_read_word
#define pgm_read_dword_near pgm_read_dword
#define pgm_read_ptr_near pgm_read_ptr
#define pgm_read_byte_far pgm_read_byte
#define pgm_read_word_far pgm_read_word
#define pgm_read_dword_far pgm_read_dword
#define pgm_get_far_address(x) ((uint32_t)(uintptr_t)&(x))
#define memcpy_P memcpy
#define strlen_P strlen
static inline size_t strnlen_P(const char *s, size_t n)
{
  size_t i = 0;
  while (i < n && s[i])
    i++;
  return i;
}
typedef uint32_t uint_farptr_t;
#define strlen_PF(a) strlen((const char *)(uintptr_t)(a))
#define strcpy_P strcpy
#define strncpy_P strncpy
#define strcmp_P strcmp
#define strncmp_P strncmp
#define memcmp_P memcmp
#define memccpy_P memccpy
#define strcat_P strcat
#endif
