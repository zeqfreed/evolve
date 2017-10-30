#pragma once

#include "utils/memory.h"

typedef struct Dresser {
  PlatformAPI *platformAPI;
  MemoryArena *mainArena;
  MemoryArena *tempArena;
} Dresser;

#define MAX_SKIN_IDX 8
#define MAX_FACE_IDX 8
#define MAX_HAIR_COLOR_IDX 9
#define MIN_FACIAL_DETAIL_IDX 4
#define MAX_FACIAL_DETAIL_IDX 14

typedef struct CharAppearance {
  int32_t skinIdx;
  int32_t faceIdx;
  int32_t hairColorIdx;
  int32_t facialDetailIdx;
} CharAppearance;
