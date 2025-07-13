#pragma once

#include "fs.h"

typedef struct PlatformFile {
  MacosOpenFile _private;
} PlatformFile;

typedef struct DirectoryListingIter {
  void *data;
} DirectoryListingIter;

typedef MacosDirectoryListingEntry DirectoryListingEntry;

#include "platform/mpq.h"

typedef MPQFile LoadedAsset;
typedef MPQFileId AssetId;

typedef struct SoundBuffer {
  uint32_t channels;
  uint32_t sample_rate;
  uint32_t length;
  void *_private;
} SoundBuffer;

#define ASSET_IDS_EQUAL(A, B) ((A.hash == B.hash) && (A.check1 == B.check1) && (A.check2 == B.check2))
