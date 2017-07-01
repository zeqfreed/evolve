#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "keyboard.h"

#ifdef __cplusplus
    #define C_LINKAGE extern "C"
#else
    #define C_LINKAGE extern
#endif

#ifdef DEBUG
#define ASSERT(x) if (!(x)) { printf("Assertion at %s, line %d failed: %s\n", __FILE__, __LINE__, #x); *((uint32_t *)1) = 0xDEADCAFE; }
#else
#define ASSERT(...)
#endif

#ifdef _MSC_VER
#define PACKED __declspec(align(1))
#else
#define PACKED __attribute__((packed))
#endif

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define PAPI_OK(x) (x >= 0)
#define PAPI_ERROR(x) (x < 0)

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

typedef int32_t (* GetFileSizeFunc)(char *);
typedef int32_t (* ReadFileContentsFunc)(char *, void*, uint32_t);
typedef void *(* AllocateMemoryFunc)(size_t size);
typedef void ( *TerminateFunc)();

typedef struct PlatformAPI {
  GetFileSizeFunc get_file_size;
  ReadFileContentsFunc read_file_contents;
  AllocateMemoryFunc allocate_memory;
  TerminateFunc terminate;
} PlatformAPI;

typedef struct GlobalState {
  PlatformAPI platform_api;
  KeyboardState *keyboard;
  void *state;
} GlobalState;

typedef void (* DrawFrameFunc)(GlobalState *state, DrawingBuffer *buffer, float dt);
