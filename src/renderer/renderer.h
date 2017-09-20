#pragma once

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "platform/platform.h"
#include "utils/memory.h"
#include "utils/texture.h"
#include "utils/math.h"

typedef uint16_t zval_t;
#define ZBUFFER_MIN 0
#define ZBUFFER_MAX 0xFFFF
#define ZTEST(new, old) ((new > old))

#define WHITE {1.0f, 1.0f, 1.0f}
#define BLACK {0.0f, 0.0f, 0.0f}
#define RED {1.0f, 0.0f, 0.0f}
#define GREEN {0.0f, 1.0f, 0.0f}
#define BLUE {0.0f, 0.0f, 1.0f}

struct RenderingContext;

#define FRAGMENT_FUNC(name) bool name(RenderingContext *ctx, void *shader_data, uint32_t x, uint32_t y, float t0, float t1, float t2, Texel *color)
typedef FRAGMENT_FUNC(FragmentFunc);

#define DRAW_TRIANGLE_FUNC(name) void name(RenderingContext *ctx, FragmentFunc *fragment, void *shader_data, Vec3f p0, Vec3f p1, Vec3f p2)
typedef DRAW_TRIANGLE_FUNC(DrawTriangleFunc);

#define DRAW_LINE_FUNC(name) void name(RenderingContext *ctx, Vec3f p0, Vec3f p1, Vec4f color)
typedef DRAW_LINE_FUNC(DrawLineFunc);

typedef struct ShaderContext {
  Vec3f positions[3];
} ShaderContext;

typedef struct RenderingContext {
  void *target;
  uint32_t target_width;
  uint32_t target_height;
  zval_t *zbuffer;

  DrawTriangleFunc *draw_triangle;
  DrawLineFunc *draw_line;

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
