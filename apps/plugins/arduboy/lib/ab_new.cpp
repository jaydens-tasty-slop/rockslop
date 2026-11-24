/* SPDX-License-Identifier: GPL-2.0-or-later */
/* operator new/delete for the few sketches that allocate objects: a small
 * static pool with per-size free lists (blocks are rounded up to 8 bytes and
 * only ever reused for the same size). Link it into such a game only. */
#include <stddef.h>
#include <stdint.h>

#ifndef AB_NEW_POOL
#define AB_NEW_POOL 4096
#endif

namespace {
struct Block { Block *next; };
const size_t QUANTUM = 8;
const size_t CLASSES = 16;      /* sizes 8 .. 128 */

alignas(8) uint8_t pool[AB_NEW_POOL];
size_t used;
Block *freeList[CLASSES];
Block *bigList;                 /* freed blocks above 128 bytes, first fit */
}

void *operator new(size_t size)
{
  size_t cls = (size + QUANTUM - 1) / QUANTUM;
  if (cls == 0)
    cls = 1;
  if (cls > CLASSES) {
    for (Block **l = &bigList; *l; l = &(*l)->next) {
      if (*(size_t *)((uint8_t *)*l - QUANTUM) >= cls) {
        Block *r = *l;
        *l = r->next;
        return r;
      }
    }
    size_t need = QUANTUM + cls * QUANTUM;
    if (used + need > sizeof(pool))
      return NULL;
    uint8_t *p = pool + used;
    used += need;
    *(size_t *)p = cls;
    return p + QUANTUM;
  }
  Block *b = freeList[cls - 1];
  if (b) {
    freeList[cls - 1] = b->next;
    return b;
  }
  /* an 8 byte header holds the size class */
  size_t need = QUANTUM + cls * QUANTUM;
  if (used + need > sizeof(pool))
    return NULL;
  uint8_t *p = pool + used;
  used += need;
  *(size_t *)p = cls;
  return p + QUANTUM;
}

void operator delete(void *p)
{
  if (!p)
    return;
  size_t cls = *(size_t *)((uint8_t *)p - QUANTUM);
  Block *b = (Block *)p;
  Block **l = cls > CLASSES ? &bigList : &freeList[cls - 1];
  b->next = *l;
  *l = b;
}

void operator delete(void *p, size_t) { operator delete(p); }

void *operator new[](size_t size) { return operator new(size); }
void operator delete[](void *p) { operator delete(p); }
void operator delete[](void *p, size_t) { operator delete(p); }
