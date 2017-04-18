#include "platform/platform.h"
#include "utils/memory.h"

#include "assets.h"

LoadedFile load_file(PlatformAPI *api, MemoryArena *arena, char *filename)
{
  LoadedFile result = {};

  int32_t file_size = api->get_file_size(filename);
  if (PAPI_ERROR(file_size)) {
    printf("Failed to get size of file %s\n", filename);
    return result;
  }

  void *memory = arena->allocate(file_size);
  ASSERT(memory);
  
  if (PAPI_OK(api->read_file_contents(filename, memory, file_size))) {
    printf("Read file %s of %d bytes\n", filename, file_size);
  } else {
    printf("Failed to read file %s\n", filename);
    return result;
  }

  result.contents = memory;
  result.size = file_size;
  return result;
}
