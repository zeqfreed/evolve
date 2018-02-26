#pragma once

#define KB(v) (1024 * v)
#define MB(v) (1024 * KB(v))
#define GB(v) (1024 * MB(v))

typedef struct MemoryArena {
  size_t total_size;
  size_t taken;
  void *memory;

  static MemoryArena *initialize(void *memory, size_t size);
  void *allocate(size_t size);
  MemoryArena *subarena(size_t size);
  void discard();
} MemoryArena;

typedef struct MemoryAllocator {
  PlatformAPI *papi;
  uint32_t total_allocated;
} MemoryAllocator;

#define ALLOCATE_SIZE(A, S) (memory_allocate(A, S))
#define ALLOCATE_MANY(A, T, C) ((T *) (memory_allocate(A, sizeof(T) * C)))
#define ALLOCATE_ONE(A, T) ALLOCATE_MANY(A, T, 1)
