#pragma once

#define KB(v) (1024 * v)
#define MB(v) (1024 * KB(v))
#define GB(v) (1024 * MB(v))

typedef struct MemoryArena {
  size_t total_size;
  size_t taken;
  void *memory;

  void *allocate(size_t size);
  static MemoryArena *initialize(void *memory, size_t size);
} MemoryArena;
