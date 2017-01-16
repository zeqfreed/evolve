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

typedef struct RenderingContext {
  DrawingBuffer *target;
  zval_t *zbuffer;

  Texture *diffuse;
  Texture *normal;
  Texture *shadowmap;

  Vec3f clear_color;
  Vec3f light;

  Vec4f near_clip_plane;

  Mat44 model_mat;
  Mat44 view_mat;
  Mat44 projection_mat;
  Mat44 viewport_mat;
  Mat44 shadow_mvp_mat;

  Mat44 mvp_mat;
  Mat44 modelview_mat;
  Mat44 normal_mat;
  Mat44 shadow_mat;
} RenderingContext;

struct IShader {
  Vec3f positions[3];

  virtual ~IShader() {};
  virtual void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color) = 0;
  virtual bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color) = 0;
};
