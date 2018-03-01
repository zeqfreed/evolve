#pragma once

#include <stdio.h>
#include <stdint.h>

#define DIRECTORY_SEPARATOR ('\\')

typedef struct WindowsFile {
  HANDLE handle;
} WindowsFile;

typedef struct WindowsDirectoryListingEntry {
  char *name;
  bool is_dir;
} WindowsDirectoryListingEntry;

typedef struct WindowsDirectoryListingIter {
  bool is_first;
  HANDLE handle;
  WindowsDirectoryListingEntry entry;
} WindowsDirectoryListingIter;
