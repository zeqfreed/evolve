
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
  bool result = (pixelsRead < image->header.width * image->header.height);
  return result;
}

Vec3f TgaPixelIter::next()
{
  Vec3f result = {};

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
        rleCount = packet & 127;

        uint8_t b = pixelData[0];
        uint8_t g = pixelData[1];
        uint8_t r = pixelData[2];
        pixelData += bpp;

        rleValue = (Vec3f){r / 255.0, g / 255.0, b / 255.0};
      } else {
        rawCount = packet & 127;
      }
    }

    if (rleCount--) {
      result = rleValue;
    } else if (rawCount--) {
      uint8_t b = pixelData[0];
      uint8_t g = pixelData[1];
      uint8_t r = pixelData[2];
      pixelData += bpp;

      result = (Vec3f){r / 255.0, g / 255.0, b / 255.0};
    }

  } else {
    uint8_t b = pixelData[0];
    uint8_t g = pixelData[1];
    uint8_t r = pixelData[2];
    pixelData += bpp;

    result = (Vec3f){r / 255.0, g / 255.0, b / 255.0};
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

  while (iter.hasMore()) {
    Vec3f pixel = iter.next();
    //printf("(%d, %d): %.3f %.3f %.3f\n", iter.x, iter.y, pixel.r, pixel.g, pixel.b);
    texture->pixels[iter.y * header.width + iter.x] = pixel;
  }
}
