#pragma once

PACK_START(1);

typedef struct BlpHeader {
  uint8_t    ident[4];           // "BLP2" magic number
  uint32_t   version;               // 0 = JPG, 1 = BLP / DXTC / Uncompressed
  uint8_t    compression;        // 1 = BLP, 2 = DXTC, 3 = Uncompressed
  uint8_t    alphaDepth;        // 0, 1, 4, or 8
  uint8_t    alphaType;         // 0, 1, 7, or 8
  uint8_t    hasMips;           // 0 = no mips, 1 = has mips
  uint32_t   width;              // Image width in pixels, usually a power of 2
  uint32_t   height;             // Image height in pixels, usually a power of 2
  uint32_t   mipmapOffsets[16]; // The file offsets of each mipmap, 0 for unused
  uint32_t   mipmapLengths[16]; // The length of each mipmap data block
  uint32_t   palette[256];       // A set of 256 ARGB values used as a color palette
} PACKED BlpHeader;

PACK_END();

typedef struct BlpImage {
  BlpHeader header;

  void read_header(void *bytes, size_t size);
  void read_into_texture(void *bytes, size_t size, Texture *texture);
} BlpImage;

typedef struct BlpPixelIter {
  bool initialized;

  BlpImage *image;
  uint8_t *pixelData;
  uint8_t *alphaData;
  uint32_t pixelsRead;

  uint32_t x;
  uint32_t y;

  BlpPixelIter(BlpImage *image, uint8_t *pixelData);
  bool hasMore();
  Vec4f next();
} BlpPixelIter;

typedef struct BlpBlockIter {
  bool initialized;
  uint32_t blocksRead;
  uint32_t blocksTotal;
  BlpImage *image;
  uint8_t *data;

  Vec4f colors[16];
  uint32_t x;
  uint32_t y;

  BlpBlockIter(BlpImage *image, uint8_t *data);
  bool hasMore();
  Vec4f *next();
} BlpBlockIter;
