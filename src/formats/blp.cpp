#include "blp.h"

void BlpImage::read_header(void *bytes, size_t size)
{
    header = *(BlpHeader *) bytes;
}

BlpPixelIter::BlpPixelIter(BlpImage *image, uint8_t *pixelData)
{
  ASSERT(image->header.compression == 1);

  initialized = false;
  x = 0;
  y = 0;

  this->pixelsRead = 0;
  this->image = image;
  this->pixelData = pixelData;
  this->alphaData = pixelData + sizeof(uint8_t) * image->header.width * image->header.height;
}

bool BlpPixelIter::hasMore()
{
  bool result = (pixelsRead < (uint32_t) (image->header.width * image->header.height));
  return result;
}

Vec4f BlpPixelIter::next()
{
  Vec4f result = {};

  if (initialized) {
    x++;
    if (x >= image->header.width) {
      x = 0;
      y++;
    }
  } else {
    initialized = true;
  }

  uint8_t colorIdx = pixelData[pixelsRead];
  uint32_t color = image->header.palette[colorIdx];

  result.a = ((color & 0xFF000000) >> 24) / 255.0;
  result.r = ((color & 0x00FF0000) >> 16) / 255.0;
  result.g = ((color & 0x0000FF00) >> 8) / 255.0;
  result.b = (color & 0x000000FF) / 255.0;

  if (image->header.alphaDepth == 8) {
    result.a = alphaData[pixelsRead] / 255.0;
  } else if (image->header.alphaDepth == 1) {
    uint8_t bit_idx = pixelsRead & 7;
    if ((alphaData[pixelsRead / 8] & (1 << bit_idx)) > 0) {
      result.a = 1.0f;
    } else {
      result.a = 0.0f;
    }
  } else {
    result.a = 1.0f;
  }

  pixelsRead++;
  return result;
}

BlpBlockIter::BlpBlockIter(BlpImage *image, uint8_t *data)
{
  initialized = false;
  this->image = image;
  this->data = data;

  x = 0;
  y = 0;
  blocksRead = 0;
  blocksTotal = ((image->header.width * image->header.height + 3) & ~0x3) / 16;

  for (size_t i = 0; i < 16; i++) {
    colors[i] = {1.0f, 1.0f, 1.0f, 1.0f};
  }
}

bool BlpBlockIter::hasMore()
{
  bool result = (blocksRead < blocksTotal);
  return result;
}

static inline Vec3f rgb565(uint16_t rgb)
{
  float r = ((rgb >> 11) & 0x1F) / 31.0;
  float g = ((rgb >> 5) & 0x3f) / 63.0;
  float b = (rgb & 0x1F) / 31.0;
  return {r, g, b};
}

Vec4f *BlpBlockIter::next()
{
  if (initialized) {
    y += 4;
    if (y >= image->header.height) {
      y = 0;
      x += 4;
    }
  } else {
    initialized = true;
  }

  if (image->header.alphaType == 1) {
    for (size_t i = 0; i < 4; i++) {
      uint16_t row = data[i*2] + (data[i*2 + 1] << 8);
      for (size_t j = 0; j < 4; j++) {
        float alpha = ((row >> (j*4)) & 0x0F) / 15.0;
        colors[i * 4 + j].a = alpha;
      }
    }

    data += 8;
  }

  uint16_t c0 = data[0] + data[1] * 256;
  uint16_t c1 = data[2] + data[3] * 256;

  Vec4f c[4] = {};
  c[0] = Vec4f(rgb565(c0), 1.0f);
  c[1] = Vec4f(rgb565(c1), 1.0f);

  if (c0 > c1 || image->header.alphaType == 1) {
    c[2] = c[0] * (2.0f / 3.0f) + c[1] * (1.0f / 3.0f);
    c[3] = c[0] * (1.0f / 3.0f) + c[1] * (2.0f / 3.0f);
  } else {
    c[2] = c[0] * 0.5 + c[1] * 0.5;
    c[3] = (image->header.alphaDepth == 1) ? Vec4f(0.0f, 0.0f, 0.0f, 0.0f) : Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
  }

  for (int32_t i = 0; i < 4; i++) {
    for (int32_t j = 0; j < 4; j++) {
      uint8_t codes = data[4 + i];
      uint8_t lookup = (codes >> (j * 2)) & 0x3;

      if (image->header.alphaDepth == 1) {
        colors[i * 4 + j] = c[lookup];
      } else {
        colors[i * 4 + j].xyz = c[lookup].xyz;
      }
    }
  }

  data += 8;

  blocksRead++;

  return &colors[0];
}


void BlpImage::read_into_texture(void *bytes, size_t size, Texture *texture)
{
  read_header(bytes, size);

  ASSERT(header.compression == 1 || header.compression == 2);

  ASSERT(texture->width == header.width);
  ASSERT(texture->height == header.height);

  uint32_t offset = header.mipmapOffsets[0];

  if (header.compression == 1) {
    uint8_t *pixelData = (uint8_t *) ((uint8_t *)bytes + offset);
    BlpPixelIter iter = BlpPixelIter(this, pixelData);

    while (iter.hasMore()) {
      Vec4f pixel = iter.next();
      texture->pixels[iter.y * header.width + iter.x] = pixel;
    }

  } else if (header.compression == 2) {
    uint8_t *data = (uint8_t *) ((uint8_t *)bytes + offset);
    BlpBlockIter iter = BlpBlockIter(this, data);

    while (iter.hasMore()) {
      Vec4f *colors = iter.next();

      uint32_t idx = 0;

      for (uint32_t x = iter.x; x < iter.x + 4; x++) {
        for (uint32_t y = iter.y; y < iter.y + 4; y++) {
          if (x < header.width && y < header.height) {
            texture->pixels[x * header.height + y] = colors[idx];
          }

          idx++;
        }
      }
    }
  }
}
