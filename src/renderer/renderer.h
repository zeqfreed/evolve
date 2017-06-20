#pragma once

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "platform/platform.h"
#include "memory.h"
#include "math.h"
#include "tga.h"

typedef uint16_t zval_t;
#define ZBUFFER_MIN 0
#define ZBUFFER_MAX 0xFFFF
#define ZTEST(new, old) ((new > old))

#define WHITE (Vec3f){1, 1, 1}
#define BLACK (Vec3f){0, 0, 0}
#define RED (Vec3f){1, 0, 0}
#define GREEN (Vec3f){0, 1, 0}
#define BLUE (Vec3f){0, 0, 1}

typedef struct ShaderContext {
  Vec3f positions[3];
} ShaderContext;

typedef struct RenderingContext {
  DrawingBuffer *target;
  zval_t *zbuffer;

  Vec3f clear_color;
  Vec3f light;

  Vec4f near_clip_plane;

  Mat44 model_mat;
  Mat44 view_mat;
  Mat44 projection_mat;
  Mat44 viewport_mat;

  Mat44 mvp_mat;
  Mat44 modelview_mat;
  Mat44 normal_mat;
} RenderingContext;

typedef bool FragmentFunc(RenderingContext *ctx, void *shader_data, uint32_t x, uint32_t y,
                          float t0, float t1, float t2, Vec3f *color);

#define FRAGMENT_FUNC(name) bool name(RenderingContext *ctx, void *shader_data, uint32_t x, uint32_t y, \
                                      float t0, float t1, float t2, Vec3f *color)
