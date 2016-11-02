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
