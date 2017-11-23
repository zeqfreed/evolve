#pragma once

#include "texture.h"

typedef struct FTDHeader {
  char magic[4];
  float fontSize;
  uint32_t lineHeight;
  uint32_t codepointsCount;
  uint32_t rangesCount;
  uint32_t rangesOffset;
  uint32_t quadsCount;
  uint32_t quadsOffset;
  uint32_t kernPairsCount;
  uint32_t kernPairsOffset;
} __attribute__((packed)) FTDHeader;

typedef struct FontQuad {
  float x0, y0;
  float x1, y1;
  float s0, t0;
  float s1, t1;
} __attribute__((packed)) FontQuad;

typedef struct Font {
  Texture *texture;
  float fontSize;
  float lineHeight;
  uint32_t rangesCount;
  uint32_t codepointsCount;
  uint32_t *ranges;
  FontQuad *quads;
  float *kernPairs;
} Font;
