#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "keyboard.h"
#include "mouse.h"

#include "defs.h"

#define PAPI_OK(x) (x >= 0)
#define PAPI_ERROR(x) (x < 0)

//#if PLATFORM == MACOS
  #include "macos/platform.h"
//#endif

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

typedef int32_t (* FileOpenFunc)(OpenFile *file, char *filename);
typedef int32_t (* FileReadFunc)(OpenFile *file, void *dst, uint32_t offset, uint32_t bytes);

typedef bool (*DirectoryListingBeginFunc)(DirectoryListingIter *iter, char *dir);
typedef DirectoryListingEntry *(*DirectoryListingNextEntryFunc)(DirectoryListingIter *iter);
typedef void (*DirectoryListingEndFunc)(DirectoryListingIter *iter);

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
} PlatformAPI;

extern PlatformAPI PLATFORM_API;

typedef struct GlobalState {
  PlatformAPI platform_api;
  KeyboardState *keyboard;
  MouseState *mouse;
  void *state;
} GlobalState;

typedef void (* DrawFrameFunc)(GlobalState *state, DrawingBuffer *buffer, float dt);
