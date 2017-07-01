#include <Windows.h>

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
