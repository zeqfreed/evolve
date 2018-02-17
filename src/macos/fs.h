#pragma once

#include <stdio.h>
#include <dirent.h>
#include <stdint.h>

#define DIRECTORY_SEPARATOR ('/')

typedef struct MacosOpenFile {
  int32_t fd;
} MacosOpenFile;

typedef struct MacosDirectoryListingIter {
  DIR *dp;
} MacosDirectoryListingIter;

typedef struct MacosDirectoryListingEntry {
  char *name;
  bool is_dir;
} MacosDirectoryListingEntry;
