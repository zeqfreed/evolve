#pragma once

#include <Windowsx.h>
#include <Windows.h>
#include <winuser.h>

#undef MB_LEFT
#undef MB_RIGHT
#undef MB_MIDDLE

#undef near // WTF Windows???
#undef far

#include "fs.h"

typedef struct PlatformFile {
  WindowsFile _private;
} PlatformFile;

typedef struct DirectoryListingIter {
  WindowsDirectoryListingIter _private;
} DirectoryListingIter;

typedef WindowsDirectoryListingEntry DirectoryListingEntry;

#include "platform/mpq.h"

typedef MPQFile LoadedAsset;
typedef MPQFileId AssetId;

#define ASSET_IDS_EQUAL(A, B) ((A.hash == B.hash) && (A.check1 == B.check1) && (A.check2 == B.check2))
