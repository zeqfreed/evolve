#pragma once

typedef struct Texture{
    uint32_t width;
    uint32_t height;
    Vec3f *pixels;
} Texture;

#define TGA_ATTRIBUTES_PER_PIXEL_MASK 0b111

typedef struct TgaHeader
{
  uint8_t idLength;
  uint8_t colorMapType;
  uint8_t imageType;
  uint16_t colorMapStart;
  uint16_t colorMapLength;
  uint8_t colorMapDepth;
  uint16_t xOffset;
  uint16_t yOffset;
  uint16_t width;
  uint16_t height;
  uint8_t pixelDepth;
  uint8_t imageDescriptor;
} __attribute__((packed)) TgaHeader;

typedef struct TgaImage {
  TgaHeader header;
  bool flipX;
  bool flipY;

  void read_header(void *bytes, size_t size);
  void read_into_texture(void *bytes, size_t size, Texture *texture);
} TgaImage;

typedef struct TgaPixelIter {
  bool initialized;

  TgaImage *image;
  uint8_t *pixelData;
  uint32_t pixelsRead;
  bool compressed;
  int rleCount;
  int rawCount;
  Vec3f rleValue;

  int bpp;

  int x;
  int y;
  int yinc;

  TgaPixelIter(TgaImage *image, uint8_t *pixelData);
  bool hasMore();
  Vec3f next();

private:
 
} TgaPixelIter;
