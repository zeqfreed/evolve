
#include <stdint.h>

#ifdef __cplusplus
    #define C_LINKAGE extern "C"
#else
    #define C_LINKAGE extern
#endif

typedef struct DrawingBuffer {
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bits_per_pixel;  
  void *pixels;
} DrawingBuffer;

typedef struct FileContents {
  uint32_t size;
  void *bytes;
} FileContents;

typedef FileContents (* ReadFileContentsFunc)(char *);
typedef void *(* AllocateMemoryFunc)(size_t size);

typedef struct PlatformAPI {
  ReadFileContentsFunc read_file_contents;
  AllocateMemoryFunc allocate_memory;
} PlatformAPI;

typedef struct GlobalState {
  PlatformAPI platform_api;
  void *state;
} GlobalState;

typedef void (* DrawFrameFunc)(GlobalState *state, DrawingBuffer *buffer);
