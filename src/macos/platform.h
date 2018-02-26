#pragma once

#include "fs.h"

typedef struct OpenFile {
  MacosOpenFile _private;
} OpenFile;

typedef struct DirectoryListingIter {
  void *data;
} DirectoryListingIter;

typedef MacosDirectoryListingEntry DirectoryListingEntry;

#include "platform/mpq.h"

typedef MPQFile LoadedAsset;
typedef MPQFileId AssetId;

#define ASSET_IDS_EQUAL(A, B) ((A.hash == B.hash) && (A.check1 == B.check1) && (A.check2 == B.check2))
