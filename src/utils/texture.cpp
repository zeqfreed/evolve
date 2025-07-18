#include "texture.h"

Texture *texture_create(MemoryArena *arena, uint32_t width, uint32_t height)
{
  Texture *result = (Texture *) arena->allocate(sizeof(Texture));
  result->width = width;
  result->height = height;
  result->pixels = (Texel *) arena->allocate(sizeof(Texel) * width * height);

  return result;
}

static inline uint32_t rgba_color(Vec4f color)
{
  uint8_t r = (uint8_t) (255.0 * color.r);
  uint8_t g = (uint8_t) (255.0 * color.g);
  uint8_t b = (uint8_t) (255.0 * color.b);

#if COLOR_BGR
  return (0xFF000000 | (r << 16) | (g << 8) | b);
#else
  return (0xFF000000 | (b << 16) | (g << 8) | r);
#endif
}

static inline Vec4f color_rgba(uint32_t color)
{
#if COLOR_BGR
  float b = (color & 0xFF) / 255.0;
  float g = ((color >> 8) & 0xFF) / 255.0;
  float r = ((color >> 16) & 0xFF) / 255.0;
#else
  float r = (color & 0xFF) / 255.0;
  float g = ((color >> 8) & 0xFF) / 255.0;
  float b = ((color >> 16) & 0xFF) / 255.0;
#endif

  return {r, g, b, 0.0f};
}

#ifdef __ARCH_X86__
#include <emmintrin.h>

static inline void texture_clear(Texture *texture, Vec4f color)
{
  ASSERT(texture != NULL);

  uint32_t iterCount = (texture->width * texture->height) / 4;

  __m128 value = _mm_set_ps(color.x, color.y, color.z, color.a);
  __m128 *p = (__m128 *) texture->pixels;

  while (iterCount--) {
    p[0] = value;
    p[1] = value;
    p[2] = value;
    p[3] = value;
    p += 4;
  }
}

#else

static inline void texture_clear(Texture *texture, Vec4f color)
{
  ASSERT(texture != NULL);

  uint32_t iterCount = texture->width * texture->height;
  Vec4f *p = texture->pixels;

  while (iterCount--) {
    *p++ = color;
  }
}

#endif