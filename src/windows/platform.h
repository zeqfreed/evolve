#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_LEANMEAN
#define INITGUID

#include <Windowsx.h>
#include <Windows.h>
#include <winuser.h>

#undef MB_LEFT
#undef MB_RIGHT
#undef MB_MIDDLE

#include "fs.h"
#include "sound.h"

#undef near
#undef far

typedef struct PlatformFile {
  WindowsFile _private;
} PlatformFile;

typedef struct DirectoryListingIter {
  WindowsDirectoryListingIter _private;
} DirectoryListingIter;

typedef struct SoundBuffer {
  WindowsSoundBuffer _private;
} SoundBuffer;

typedef WindowsDirectoryListingEntry DirectoryListingEntry;

#include "platform/mpq.h"

typedef MPQFile LoadedAsset;
typedef MPQFileId AssetId;

#define ASSET_IDS_EQUAL(A, B) ((A.hash == B.hash) && (A.check1 == B.check1) && (A.check2 == B.check2))
