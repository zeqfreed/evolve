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
