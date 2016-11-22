#include <cstdint>

#include "platform/platform.h"
#include "memory.h"

MemoryArena *MemoryArena::initialize(void *memory, size_t size)
{
  ASSERT(size > sizeof(MemoryArena));

  MemoryArena *arena = (MemoryArena *) memory;
  arena->memory = (uint8_t *) arena + sizeof(MemoryArena);
  arena->total_size = size - sizeof(MemoryArena);
  arena->taken = 0;

  return arena;
}

void *MemoryArena::allocate(size_t size)
{
  ASSERT(size > 0);
  ASSERT(taken + size <= total_size);

  void *result = (char *) memory + taken;
  taken += size;
  return result;
}

MemoryArena *MemoryArena::subarena(size_t size)
{
  ASSERT(size > 0);

  size_t required_size = size + sizeof(MemoryArena);
  ASSERT(taken + required_size <= total_size);

  void *mem = this->allocate(required_size);
  return MemoryArena::initialize(mem, required_size);
}
