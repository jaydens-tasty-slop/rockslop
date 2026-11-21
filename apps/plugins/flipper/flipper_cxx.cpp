/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Minimal C++ runtime for ported games: no exceptions, RTTI or libstdc++. */
#include <stdlib.h>
#include <new>
#include <furi.h>

void *operator new(size_t n) noexcept { return malloc(n); }
void *operator new[](size_t n) noexcept { return malloc(n); }
void operator delete(void *p) noexcept { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete[](void *p, size_t) noexcept { free(p); }

extern "C" void __cxa_pure_virtual() { for (;;) {} }

/* libstdc++ containers report failure through these; there is no unwinding. */
namespace std {
void __throw_bad_alloc() { for (;;) {} }
void __throw_length_error(const char *) { for (;;) {} }
void __throw_bad_array_new_length() { for (;;) {} }
}

/* assert() from the Rockbox libc reaches panicf() with C++ linkage here. */
void panicf(const char *, ...) { furi_crash("assertion failed"); }
