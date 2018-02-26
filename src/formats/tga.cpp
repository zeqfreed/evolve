#include <cstdint>
#include "tga.h"

TgaPixelIter::TgaPixelIter(TgaImage *image, uint8_t *pixelData)
{
  initialized = 0;
   x = 0;
   y = 0;
   yinc = 1;

   this->image = image;

   if (image->flipX) {
     x = image->header.width - 1;
   }

   if (image->flipY) {
     y = image->header.height - 1;
     yinc = -1;
   }

   compressed = (image->header.imageType == 10);
   rleCount = 0;
   rawCount = 0;
   pixelsRead = 0;
   bpp = image->header.pixelDepth / 8;

   this->pixelData = pixelData;
}

bool TgaPixelIter::hasMore()
{
  bool result = (pixelsRead < (uint32_t) (image->header.width * image->header.height));
  return result;
}

Vec4f TgaPixelIter::next()
{
  Vec4f result = {};

  if (initialized) {
    if (image->flipX) {
      x--;
      if (x < 0) {
        x = image->header.width - 1;
        y += yinc;
      }
    } else {
      x++;
      if (x >= image->header.width) {
        x = 0;
        y += yinc;
      }
    }
  } else {
    initialized = true;
  }

  if (compressed) {
    if (!rleCount && !rawCount) {
      uint8_t packet = *pixelData++;
      bool isRLE = packet & (1 << 7);

      if (isRLE) {
        rleCount = (packet & 127) + 1;

        uint8_t b = pixelData[0];
        uint8_t g = pixelData[1];
        uint8_t r = pixelData[2];
        uint8_t a = pixelData[3];
        pixelData += bpp;

        rleValue = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
      } else {
        rawCount = (packet & 127) + 1;
      }
    }

    if (rleCount > 0) {
      rleCount--;
      result = rleValue;

    } else if (rawCount > 0) {
      rawCount--;

      uint8_t b = pixelData[0];
      uint8_t g = pixelData[1];
      uint8_t r = pixelData[2];
      uint8_t a = pixelData[3];
      pixelData += bpp;

      result = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }

  } else {
    uint8_t b = pixelData[0];
    uint8_t g = pixelData[1];
    uint8_t r = pixelData[2];
    uint8_t a = pixelData[3];
    pixelData += bpp;

    result = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
  }

  pixelsRead++;
  return result;
}

void TgaImage::read_header(void *bytes, size_t size)
{
    header = *(TgaHeader *) bytes;
    flipX = (header.imageDescriptor & 0b010000) > 0;
    flipY = (header.imageDescriptor & 0b100000) > 0;

    ASSERT(header.idLength == 0);
    ASSERT(header.colorMapType == 0);
    ASSERT(header.imageType == 10 || header.imageType == 2);
    ASSERT(header.pixelDepth == 24 || header.pixelDepth == 32);
}

void TgaImage::read_into_texture(void *bytes, size_t size, Texture *texture)
{
  read_header(bytes, size);

  ASSERT(texture->width == header.width);
  ASSERT(texture->height == header.height);

  uint8_t *pixelData = (uint8_t *) ((uint8_t *)bytes + sizeof(TgaHeader)); // Doesn't account for ImageID or Palette
  TgaPixelIter iter = TgaPixelIter(this, pixelData);

  ASSERT(iter.bpp == 4);

  while (iter.hasMore()) {
    Vec4f pixel = iter.next();
    texture->pixels[iter.y * header.width + iter.x] = pixel;
  }
}
