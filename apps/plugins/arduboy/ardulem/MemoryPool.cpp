#include "MemoryPool.h"

// The big array to store the lems and map modifs
unsigned char MemoryPool::Pool[MEMORY_POOL_SIZE] __attribute__((aligned(4)));
