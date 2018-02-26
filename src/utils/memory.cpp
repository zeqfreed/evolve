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

void MemoryArena::discard()
{
  taken = 0;
}

void memory_allocator_init(MemoryAllocator *allocator, PlatformAPI *papi)
{
  allocator->papi = papi;
  allocator->total_allocated = 0;
}

void *memory_allocate(MemoryAllocator *allocator, uint32_t size)
{
  size += sizeof(uint32_t);
  uint32_t *memory = (uint32_t *) allocator->papi->allocate_memory(size);
  memory[0] = size;
  allocator->total_allocated += size;

  return (void *) &memory[1];
}

void memory_free(MemoryAllocator *allocator, void *memory)
{
  uint32_t size = *((uint32_t *) memory - 1);
  allocator->total_allocated -= size;
  allocator->papi->free_memory(memory);
}
