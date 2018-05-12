#include "fs.h"

int32_t windows_fs_size(char *filename)
{
  int32_t result = -1;

  HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fileHandle == INVALID_HANDLE_VALUE) {
    return result;
  }

  uint64_t size;
  if (GetFileSizeEx(fileHandle, (LARGE_INTEGER *) &size)) {
    result = (int32_t) size;
  }

  CloseHandle(fileHandle);
  return (int32_t) size;
}

int32_t windows_fs_read(char *filename, void *memory, uint32_t memorySize)
{
  HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fileHandle == INVALID_HANDLE_VALUE) {
    return -1;
  }

  uint64_t fileSize;
  if (!GetFileSizeEx(fileHandle, (LARGE_INTEGER *) &fileSize)) {
    CloseHandle(fileHandle);
    return -1;
  }

  if (fileSize > memorySize) {
    CloseHandle(fileHandle);  
    return -1;
  }

  void *buffer = memory;
  uint32_t bytesRead = 0;
  if (!ReadFile(fileHandle, buffer, (uint32_t) fileSize, (LPDWORD) &bytesRead, NULL)) {
      CloseHandle(fileHandle);
      return -1;
  }
  
  CloseHandle(fileHandle);
  return (int32_t) bytesRead;
}

int32_t windows_file_open(WindowsFile *file, char *filename)
{
  HANDLE handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    // TODO: Handle fopen failure
    return -1;
  }

  file->handle = handle;
  return 0;
}

int32_t windows_file_read(WindowsFile *file, void *dst, uint32_t offset, uint32_t bytes)
{
  size_t bytes_left = bytes;
  size_t total_read = 0;

  void *buf = dst;

  OVERLAPPED overlapped = {0};
  overlapped.Offset = offset;

  while (total_read < bytes) {
    if (!ReadFile(file->handle, buf, bytes_left, NULL, &overlapped)) {
      if (GetLastError() != ERROR_IO_PENDING) {
        // TODO: Log
        return -1;
      }
    }

    uint32_t bytes_read;
    if (!GetOverlappedResult(file->handle, &overlapped, (LPDWORD) &bytes_read, true)) {
      // TODO: Log
      return -1;
    }

    if (bytes_read > 0) {
      bytes_left -= bytes_read;
      buf = (uint8_t *) buf + bytes_read;
      total_read += bytes_read;
    } else if (bytes_read < 0) {
      return total_read;
    } else if (bytes_read == 0) {
      break;
    }

    offset += bytes_read;
    overlapped.Offset = offset;
  }

  return total_read;
}

bool windows_directory_listing_begin(WindowsDirectoryListingIter *iter, char *directory)
{
  WIN32_FIND_DATAA fd = {0};

  char buf[MAX_PATH];
  snprintf(buf, MAX_PATH, "%s%c%s", directory, DIRECTORY_SEPARATOR, "*");
  HANDLE handle = FindFirstFileA(buf, &fd);
  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  iter->handle = handle;
  iter->is_first = true;

  iter->entry.name = fd.cFileName;
  iter->entry.is_dir = ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
  
  return true;
}

WindowsDirectoryListingEntry *windows_directory_listing_next_entry(WindowsDirectoryListingIter *iter)
{
  if (iter->is_first) {
    iter->is_first = false;
    return &iter->entry;
  }

  WIN32_FIND_DATAA fd = {0};
  if (!FindNextFileA(iter->handle, &fd)) {
    return NULL;
  }

  iter->entry.name = fd.cFileName;
  iter->entry.is_dir = ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
  return &iter->entry;
}

void windows_directory_listing_end(WindowsDirectoryListingIter *iter)
{
  FindClose(iter->handle);
}
