#pragma once

#if defined PLATFORM_MACOS
  #include "macos/platform.h"
#elif defined PLATFORM_WINDOWS
  #include "windows/platform.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "keyboard.h"
#include "mouse.h"
#include "sound.h"

#include "defs.h"

#define PAPI_OK(x) (x >= 0)
#define PAPI_ERROR(x) (x < 0)

typedef struct DrawingBuffer {
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bytes_per_pixel;
  void *pixels;
} DrawingBuffer;

typedef struct FileContents {
  uint32_t size;
  void *bytes;
} FileContents;

typedef int32_t (* GetFileSizeFunc)(char *);
typedef int32_t (* ReadFileContentsFunc)(char *, void*, uint32_t);
typedef void *(* AllocateMemoryFunc)(size_t size);
typedef void *(* FreeMemoryFunc)(void *memory);
typedef void ( *TerminateFunc)();

typedef AssetId ( *GetAssetIdFunc)(char *name);
typedef LoadedAsset ( *LoadAssetFunc)(char *name);
typedef void ( *ReleaseAssetFunc)(LoadedAsset *asset);

typedef int32_t (* FileOpenFunc)(PlatformFile *file, char *filename);
typedef int32_t (* FileReadFunc)(PlatformFile *file, void *dst, uint32_t offset, uint32_t bytes);

typedef bool (*DirectoryListingBeginFunc)(DirectoryListingIter *iter, char *dir);
typedef DirectoryListingEntry *(*DirectoryListingNextEntryFunc)(DirectoryListingIter *iter);
typedef void (*DirectoryListingEndFunc)(DirectoryListingIter *iter);

typedef bool (*SoundBufferInitFunc)(SoundBuffer *sound_buffer, uint32_t channels, uint32_t sample_rate, float seconds);
typedef void (*SoundBufferFinalizeFunc)(SoundBuffer *sound_buffer);
typedef void (*SoundBufferPlayFunc)(SoundBuffer *sound_buffer);
typedef void (*SoundBufferStopFunc)(SoundBuffer *sound_buffer);
typedef LockedSoundBufferRegion (*SoundBufferLockFunc)(SoundBuffer *sound_buffer);
typedef void (*SoundBufferUnlockFunc)(SoundBuffer *sound_buffer, LockedSoundBufferRegion *locked_region, uint32_t advance_by);

typedef struct PlatformAPI {
  GetFileSizeFunc get_file_size;
  ReadFileContentsFunc read_file_contents;
  AllocateMemoryFunc allocate_memory;
  FreeMemoryFunc free_memory;
  TerminateFunc terminate;
  GetAssetIdFunc get_asset_id;
  LoadAssetFunc load_asset;
  ReleaseAssetFunc release_asset;
  FileOpenFunc file_open;
  FileReadFunc file_read;
  DirectoryListingBeginFunc directory_listing_begin;
  DirectoryListingNextEntryFunc directory_listing_next_entry;
  DirectoryListingEndFunc directory_listing_end;

  SoundBufferInitFunc sound_buffer_init;
  SoundBufferFinalizeFunc sound_buffer_finalize;
  SoundBufferPlayFunc sound_buffer_play;
  SoundBufferStopFunc sound_buffer_stop;
  SoundBufferLockFunc sound_buffer_lock;
  SoundBufferUnlockFunc sound_buffer_unlock;
} PlatformAPI;

extern PlatformAPI PLATFORM_API;

typedef struct GlobalState {
  PlatformAPI platform_api;
  KeyboardState *keyboard;
  MouseState *mouse;
  void *state;
} GlobalState;

typedef void (* DrawFrameFunc)(GlobalState *state, DrawingBuffer *buffer, float dt);
