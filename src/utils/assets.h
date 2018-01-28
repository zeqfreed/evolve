#pragma once

typedef struct LoadedFile {
  void *contents;
  uint32_t size;
} LoadedFile;

typedef struct AssetLoader {
  PlatformAPI *papi;
  MemoryArena *tempArena;
  MemoryArena *dataArena;
} AssetLoader;
