/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The few C library functions Arduboy sketches use that a Rockbox plugin
 * does not get from the core. */
#include <stddef.h>
#include "ab_glue.h"

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return p - s;
}

char *strncpy(char *d, const char *s, size_t n)
{
    size_t i = 0;
    for (; i < n && s[i]; i++)
        d[i] = s[i];
    for (; i < n; i++)
        d[i] = 0;
    return d;
}

void *memccpy(void *dst, const void *src, int c, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) {
        if ((*d++ = *s++) == (unsigned char)c)
            return d;
    }
    return NULL;
}

int rand(void)
{
    return (int)((ab_rand() >> 8) & 0x7fff);
}

void srand(unsigned int seed)
{
    ab_srand(seed);
}

size_t strnlen(const char *s, size_t n)
{
    size_t i = 0;
    while (i < n && s[i])
        i++;
    return i;
}

char *strcpy(char *d, const char *s)
{
    char *r = d;
    while ((*d++ = *s++))
        ;
    return r;
}
