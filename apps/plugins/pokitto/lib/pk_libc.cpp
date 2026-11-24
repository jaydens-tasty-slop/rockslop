/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The few libc string functions the games call (plugins link no libc;
 * pk_glue.c uses the plugin API versions). */
#include <stddef.h>
#include <string.h>

/* The simulator links the host libc (only `new` is ours). */
#ifndef SIMULATOR

extern "C" {

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

char *strcpy(char *d, const char *s)
{
    char *r = d;
    while ((*d++ = *s++))
        ;
    return r;
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

char *strcat(char *d, const char *s)
{
    strcpy(d + strlen(d), s);
    return d;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b)
        a++, b++;
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        if (*a != *b || !*a)
            return (unsigned char)*a - (unsigned char)*b;
    }
    return 0;
}

}

extern "C" {

char *strchr(const char *s, int c)
{
    for (; *s; s++)
        if (*s == (char)c)
            return (char *)s;
    return c ? NULL : (char *)s;
}

char *strstr(const char *h, const char *n)
{
    size_t len = strlen(n);
    for (; *h; h++)
        if (!strncmp(h, n, len))
            return (char *)h;
    return len ? NULL : (char *)h;
}

int atoi(const char *s)
{
    int sign = 1, v = 0;
    while (*s == ' ')
        s++;
    if (*s == '-' || *s == '+')
        sign = *s++ == '-' ? -1 : 1;
    while (*s >= '0' && *s <= '9')
        v = v * 10 + *s++ - '0';
    return sign * v;
}

}

/* free() is a no-op with the bump allocator below. */
extern "C" void free(void *) {}
#endif /* !SIMULATOR */

/* Heap for the few `new` calls: a small bump allocator, never freed. */
static char heap[2048] __attribute__((aligned(4)));
static size_t heap_used;

void *operator new(size_t n)
{
    n = (n + 3) & ~3u;
    if (heap_used + n > sizeof(heap))
        return NULL;
    void *p = heap + heap_used;
    heap_used += n;
    return p;
}

void *operator new[](size_t n) { return operator new(n); }
void operator delete(void *) {}
void operator delete[](void *) {}
void operator delete(void *, size_t) {}
void operator delete[](void *, size_t) {}
