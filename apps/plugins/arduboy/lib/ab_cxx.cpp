/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Minimal C++ runtime pieces (the plugin links no libstdc++). Sketches do not
 * allocate; virtual destructors only need these symbols to exist. */
#include <stddef.h>

/* weak: a game that allocates links ab_new.cpp, which replaces them */
__attribute__((weak)) void operator delete(void *) {}
__attribute__((weak)) void operator delete(void *, size_t) {}

extern "C" void __cxa_pure_virtual()
{
  for (;;) {
  }
}
