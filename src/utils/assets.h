#pragma once

#include "texture.h"
#include "formats/dbc.h"
#include "formats/m2.h"
#include "formats/blp.h"

typedef struct LoadedFile {
  void *contents;
  uint32_t size;
} LoadedFile;

typedef enum AssetType {
  AT_DBC,
  AT_TEXTURE,
  AT_MODEL
} AssetType;

typedef struct Asset {
  AssetId id;
  AssetType type;
  union {
    DBCFile *dbc;
    Texture *texture;
    M2Model *model;
  };
} Asset;

typedef struct AssetNode {
  AssetNode *prev;
  AssetNode *next;
  uint32_t index;
  Asset asset;
} AssetNode;

typedef struct AssetLoader {
  PlatformAPI *papi;
  MemoryAllocator allocator;
  uint32_t assets_count;
  uint32_t max_assets;
  AssetNode **assets; // Loaded assets hashed by platform layer
  AssetNode mru_asset; // Doubly-linked most recently used assets
} AssetLoader;
