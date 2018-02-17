#include "platform.h"

#include "macos/fs.cpp"
#include "platform/mpq.cpp"

C_LINKAGE void *macos_allocate_memory(size_t size);
C_LINKAGE void *macos_free_memory(void *memory);
C_LINKAGE void macos_terminate();

MPQFile macos_load_asset(char *name);
void macos_release_asset(MPQFile *file);

PlatformAPI PLATFORM_API = {
  (GetFileSizeFunc) macos_fs_size,
  (ReadFileContentsFunc) macos_fs_read,
  (AllocateMemoryFunc) macos_allocate_memory,
  (FreeMemoryFunc) macos_free_memory,
  (TerminateFunc) macos_terminate,
  (LoadAssetFunc) macos_load_asset,
  (ReleaseAssetFunc) macos_release_asset,
  (FileOpenFunc) macos_file_open,
  (FileReadFunc) macos_file_read,
  (DirectoryListingBeginFunc) macos_directory_listing_begin,
  (DirectoryListingNextEntryFunc) macos_directory_listing_next_entry,
  (DirectoryListingEndFunc) macos_directory_listing_end,
};

MPQFile macos_load_asset(char *name)
{
  return mpq_load_file(&MPQ_REGISTRY, name);
}

void macos_release_asset(MPQFile *file)
{
  return mpq_release_file(&MPQ_REGISTRY, file);
}
