#pragma once

#include "memory.h"
#include "math.h"

typedef Vec4f Texel;

typedef struct Texture{
    uint32_t width;
    uint32_t height;
    Texel *pixels;
} Texture;

#define TEXEL4F(texture, x, y) ((Vec4f)(texture->pixels[(y) * texture->width + (x)]))
#define TEXEL3F(texture, x, y) ((Vec3f)(texture->pixels[(y) * texture->width + (x)]).xyz)

Texture *texture_create(MemoryArena *arena, uint32_t width, uint32_t height);
