/* Fixed slot allocator behind operator new/delete of the physics objects
 * (the original used the CE heap). MIT, see ../LICENSE. */
#include "util.h"
#include "tank.h"
#include "shell.h"
#include "mine.h"
#include "mine_detector.h"
#include "objlist.h"

#define SLOT_SIZE 128

static_assert(sizeof(Tank) <= SLOT_SIZE, "Tank too big for a slot");
static_assert(sizeof(Shell) <= SLOT_SIZE, "Shell too big for a slot");
static_assert(sizeof(Mine) <= SLOT_SIZE, "Mine too big for a slot");
static_assert(sizeof(MineDetector) <= SLOT_SIZE, "MineDetector too big for a slot");

static unsigned char slots[MAX_OBJECTS][SLOT_SIZE] __attribute__((aligned(8)));
static bool used[MAX_OBJECTS];

#ifndef TANKS_HOST
namespace std { const nothrow_t nothrow{}; }
#endif

void *operator new(size_t size, const std::nothrow_t &) noexcept
{
    if (size > SLOT_SIZE)
        return nullptr;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!used[i]) {
            used[i] = true;
            return slots[i];
        }
    }
    return nullptr;
}

void operator delete(void *p) noexcept
{
    if (!p)
        return;
    size_t i = ((unsigned char *)p - &slots[0][0]) / SLOT_SIZE;
    if (i < MAX_OBJECTS)
        used[i] = false;
}

void operator delete(void *p, size_t) noexcept
{
    operator delete(p);
}

extern "C" void __cxa_pure_virtual(void)
{
    for (;;) {}
}
