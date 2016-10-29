
typedef struct TgaImage {
    uint32_t width;
    uint32_t height;
    Vec3f pixels[2048*2048];
} TgaImage;

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

static void tga_read_compressed_pixels(TgaHeader *header, uint8_t *pixelData, TgaImage *image)
{
    ASSERT((header->imageDescriptor & 0b110000) == 0);

    int bpp = header->pixelDepth / 8;

    uint32_t pixelIdx = 0;
    while (pixelIdx < header->width * header->height) {
        uint8_t packet = *pixelData++;
        bool packetRLE = packet & (1 << 7);
        uint8_t packetCount = packet & 127;

        if (packetRLE) { // RLE packet
          uint8_t b = pixelData[0];
          uint8_t g = pixelData[1];
          uint8_t r = pixelData[2];
          pixelData += bpp;

          for (int i = 0; i <= packetCount; i++) {
            image->pixels[pixelIdx++] = (Vec3f){r / 255.0, g / 255.0, b / 255.0};
          }

        } else { // RAW packet
            for (int i = 0; i <= packetCount; i++) {
                uint8_t b = pixelData[0];
                uint8_t g = pixelData[1];
                uint8_t r = pixelData[2];
                pixelData += bpp;

                image->pixels[pixelIdx++] = (Vec3f){r / 255.0, g / 255.0, b / 255.0};
            } 
        }
    }    
}

static void tga_read_uncompressed_pixels(TgaHeader *header, uint8_t *pixelData, TgaImage *image)
{
    bool flipX = (header->imageDescriptor & 0b010000) > 0;
    bool flipY = (header->imageDescriptor & 0b100000) > 0;

    int x = 0;
    int y = 0;
    int yinc = 1;

    if (flipX) {
        x = header->width - 1;
    }

    if (flipY) {
        y = header->height - 1;
        yinc = -1;
    }

    int bpp = header->pixelDepth / 8;

    uint32_t pixelsRead = 0;
    while (pixelsRead < header->width * header->height) {
        uint8_t b = pixelData[0];
        uint8_t g = pixelData[1];
        uint8_t r = pixelData[2];
        pixelData += bpp;

        image->pixels[y*header->width+x] = (Vec3f){r / 255.0, g / 255.0, b / 255.0};
        pixelsRead++;

        if (flipX) {
            x--;
            if (x < 0) {
                x = header->width - 1;
                y += yinc;
            }
        } else {
            x++;
            if (x >= header->width) {
                x = 0;
                y += yinc;
            }
        }
    }
}

static void tga_read_image(void *bytes, uint32_t size, TgaImage *image)
{
    TgaHeader *header = (TgaHeader *) bytes;

    printf("TGA Image width: %d; height: %d; type: %d; bpp: %d\n",
           header->width, header->height, header->imageType, header->pixelDepth);

    uint8_t origin = (header->imageDescriptor & 0b110000) >> 4;
    printf("X offset: %d; Y offset: %d; Origin: %d\n", header->xOffset, header->yOffset, origin);

    ASSERT(header->idLength == 0);
    ASSERT(header->colorMapType == 0);
    ASSERT(header->imageType == 10 || header->imageType == 2);
    ASSERT(header->pixelDepth == 24 || header->pixelDepth == 32);

    image->width = header->width;
    image->height = header->height;

    uint8_t *pixelData = (uint8_t *) ((uint8_t *)bytes + sizeof(TgaHeader)); // Doesn't account for ImageID or Palette

    if (header->imageType == 2) {
        tga_read_uncompressed_pixels(header, pixelData, image);
    } else if (header->imageType == 10) {
        tga_read_compressed_pixels(header, pixelData, image);
    }
}
