#include <cstdlib>
#include "renderer.h"

static inline Vec4f blend_src_copy(Vec4f src, Vec4f dst)
{
  if (src.a == 0.0f) {
    return dst; // TODO: Should this be in draw_triangle?
  }

  return src;
}

static inline Texel blend_decal(Vec4f src, Vec4f dst)
{
  Vec3f c = src.rgb * src.a + dst.rgb * (1.0f - src.a);
  return Vec4f(c, 1.0f);
}

static inline Vec4f blend_src_alpha_one(Vec4f src, Vec4f dst)
{
  Vec3f c = src.rgb * src.a + dst.rgb;
  return Vec4f(c.clamped(), 1.0f);
}

static void precalculate_matrices(RenderingContext *ctx)
{
  ctx->normal_mat = ctx->model_mat.inverse().transposed();
  ctx->modelview_mat = ctx->model_mat * ctx->view_mat;

  Mat44 mvp = ctx->modelview_mat * ctx->projection_mat;
  ctx->mvp_mat = mvp;

  Mat44 m = ctx->mvp_mat;
  Vec4f p = {m.c + m.d, m.g + m.h, m.k + m.l, m.o + m.p};
  float mag = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
  ctx->near_clip_plane = p * (1.0f / mag);
}

#define OUTCODE_LEFT 1
#define OUTCODE_RIGHT 2
#define OUTCODE_BOTTOM 4
#define OUTCODE_TOP 8

static inline int32_t outcode(int32_t x, int32_t y, int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax)
{
  int32_t result = 0;

  if (x < xmin) {
    result |= OUTCODE_LEFT;
  } else if (x > xmax) {
    result |= OUTCODE_RIGHT;
  }

  if (y < ymin) {
    result |= OUTCODE_BOTTOM;
  } else if (y > ymax) {
    result |= OUTCODE_TOP;
  }

  return result;
}

static bool clip_line(int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1, int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax)
{
  int32_t code0 = outcode(*x0, *y0, xmin, ymin, xmax, ymax);
  int32_t code1 = outcode(*x1, *y1, xmin, ymin, xmax, ymax);

  while (true) {
    if (!(code0 | code1)) {
      return true; // Both inside
    } else if (code0 & code1) {
      return false; // Both outside in the same region
    } else {
      int32_t code = code0 ? code0 : code1;

      float x, y;
      float dx = *x1 - *x0;
      float dy = *y1 - *y0;

      if (code & OUTCODE_TOP) {
        x = *x0 + dx * (ymax - *y0) / dy;
        y = ymax;
      } else if (code & OUTCODE_BOTTOM) {
        x = *x0 + dx * (ymin - *y0) / dy;
        y = ymin;
      } else if (code & OUTCODE_RIGHT) {
        y = *y0 + dy * (xmax - *x0) / dx;
        x = xmax;
      } else if (code & OUTCODE_LEFT) {
        y = *y0 + dy * (xmin - *x0) / dx;
        x = xmin;
      }

      if (code == code0) {
        *x0 = x;
        *y0 = y;
        code0 = outcode(*x0, *y0, xmin, ymin, xmax, ymax);
      } else {
        *x1 = x;
        *y1 = y;
        code1 = outcode(*x1, *y1, xmin, ymin, xmax, ymax);
      }
    }
  }

  return false;
}

inline static q8 edge_funcq(q8 x0, q8 y0, q8 x1, q8 y1)
{
  return qmul(x0, y1) - qmul(x1, y0);
}

#define DRAW_LINE_TARGET_TYPE Texture
#define DRAW_LINE_TEXEL_TYPE Texel
#define DRAW_LINE_FUNC_NAME draw_line_rgba4f
#include "draw_line.cpp"

#define DRAW_LINE_TARGET_TYPE DrawingBuffer
#define DRAW_LINE_TEXEL_TYPE uint32_t
#define DRAW_LINE_TEXEL_CONVERSION_FUNC rgba_color
#define DRAW_LINE_FUNC_NAME draw_line_rgba32
#include "draw_line.cpp"

/* Texture target */

#define DRAW_TRIANGLE_TARGET_TYPE Texture
#define DRAW_TRIANGLE_TEXEL_TYPE Texel
#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba4f_noblend_nocull_nofrag
#define DRAW_TRIANGLE_BLEND 0
#define DRAW_TRIANGLE_CULL 0
#define DRAW_TRIANGLE_ZTEST 1
#define DRAW_TRIANGLE_FRAG 0
#include "draw_triangle.cpp"

/* DrawingBuffer target */

#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba32_blend_nocull_frag
#define DRAW_TRIANGLE_TARGET_TYPE DrawingBuffer
#define DRAW_TRIANGLE_TEXEL_TYPE uint32_t
#define DRAW_TRIANGLE_COLOR_TO_TEXEL(V) (color_rgba(V))
#define DRAW_TRIANGLE_TEXEL_TO_COLOR(V) (rgba_color(V))
#define DRAW_TRIANGLE_BLEND 1
#define DRAW_TRIANGLE_CULL 0
#define DRAW_TRIANGLE_ZTEST 1
#define DRAW_TRIANGLE_FRAG 1
#include "draw_triangle.cpp"

#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba32_blend_nocull_noz_frag
#define DRAW_TRIANGLE_TARGET_TYPE DrawingBuffer
#define DRAW_TRIANGLE_TEXEL_TYPE uint32_t
#define DRAW_TRIANGLE_COLOR_TO_TEXEL(V) (color_rgba(V))
#define DRAW_TRIANGLE_TEXEL_TO_COLOR(V) (rgba_color(V))
#define DRAW_TRIANGLE_BLEND 1
#define DRAW_TRIANGLE_CULL 0
#define DRAW_TRIANGLE_ZTEST 0
#define DRAW_TRIANGLE_FRAG 1
#include "draw_triangle.cpp"

#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba32_noblend_nocull_frag
#define DRAW_TRIANGLE_TARGET_TYPE DrawingBuffer
#define DRAW_TRIANGLE_TEXEL_TYPE uint32_t
#define DRAW_TRIANGLE_COLOR_TO_TEXEL(V) (color_rgba(V))
#define DRAW_TRIANGLE_TEXEL_TO_COLOR(V) (rgba_color(V))
#define DRAW_TRIANGLE_BLEND 0
#define DRAW_TRIANGLE_CULL 0
#define DRAW_TRIANGLE_ZTEST 1
#define DRAW_TRIANGLE_FRAG 1
#include "draw_triangle.cpp"

#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba32_noblend_cull_frag
#define DRAW_TRIANGLE_TARGET_TYPE DrawingBuffer
#define DRAW_TRIANGLE_TEXEL_TYPE uint32_t
#define DRAW_TRIANGLE_COLOR_TO_TEXEL(V) (color_rgba(V))
#define DRAW_TRIANGLE_TEXEL_TO_COLOR(V) (rgba_color(V))
#define DRAW_TRIANGLE_BLEND 0
#define DRAW_TRIANGLE_CULL 1
#define DRAW_TRIANGLE_ZTEST 1
#define DRAW_TRIANGLE_FRAG 1
#include "draw_triangle.cpp"

#include <emmintrin.h>

static void set_target(RenderingContext *ctx, Texture *texture)
{
  ctx->target = texture;
  ctx->target_type = TARGET_TYPE_TEXTURE;
  ctx->target_width = texture->width;
  ctx->target_height = texture->height;
  ctx->draw_triangle = &draw_triangle_rgba4f_noblend_nocull_nofrag;
  ctx->draw_line = &draw_line_rgba4f;
  ctx->blend_func = &blend_src_copy;
}

static void set_target(RenderingContext *ctx, DrawingBuffer *buffer)
{
  ctx->target = buffer;
  ctx->target_type = TARGET_TYPE_RGBA32;
  ctx->target_width = buffer->width;
  ctx->target_height = buffer->height;
  ctx->draw_triangle = &draw_triangle_rgba32_blend_nocull_frag;
  ctx->draw_line = &draw_line_rgba32;
  ctx->blend_func = &blend_src_copy;
}

static void change_draw_func(RenderingContext *ctx)
{
  switch (ctx->target_type) {
    case TARGET_TYPE_TEXTURE:
      ctx->draw_triangle = &draw_triangle_rgba4f_noblend_nocull_nofrag;
      break;

    case TARGET_TYPE_RGBA32:
      if (ctx->blending) {
        if (ctx->ztest) {
          ctx->draw_triangle = &draw_triangle_rgba32_blend_nocull_frag;
        } else {
          ctx->draw_triangle = &draw_triangle_rgba32_blend_nocull_noz_frag;
        }
      } else {
        if (ctx->culling) {
          ctx->draw_triangle = &draw_triangle_rgba32_noblend_cull_frag;
        } else {
          ctx->draw_triangle = &draw_triangle_rgba32_noblend_nocull_frag;
        }
      }
      break;
  }
}

static inline void set_blending(RenderingContext *ctx, bool enabled)
{
  bool wasEnabled = ctx->blending;
  ctx->blending = enabled;

  if (wasEnabled != enabled) {
    change_draw_func(ctx);
  }
}

static inline void set_culling(RenderingContext *ctx, bool enabled)
{
  bool wasEnabled = ctx->culling;
  ctx->culling = enabled;

  if (wasEnabled != enabled) {
    change_draw_func(ctx);
  }
}

static inline void set_ztest(RenderingContext *ctx, bool enabled)
{
  bool wasEnabled = ctx->ztest;
  ctx->ztest = enabled;

  if (wasEnabled != enabled) {
    change_draw_func(ctx);
  }
}

static void set_blend_mode(RenderingContext *ctx, BlendMode blend_mode)
{
  switch (blend_mode) {
    case BLEND_MODE_DECAL:
      ctx->blend_func = &blend_decal;
      break;

    case BLEND_MODE_SRC_COPY:
      ctx->blend_func = &blend_src_copy;
      break;

    case BLEND_MODE_SRC_ALPHA_ONE:
      ctx->blend_func = &blend_src_alpha_one;
      break;

    default:
      ctx->blend_func = &blend_src_copy;
      break;
  }
}

static inline void clear_zbuffer(RenderingContext *ctx)
{
  int width = ctx->target_width;
  int height = ctx->target_height;
  memset(ctx->zbuffer, ZBUFFER_MIN, width*height*sizeof(zval_t));
}

static Mat44 orthographic_matrix(float near, float far, float left, float bottom, float right, float top)
{
  float w = right - left;
  float h = top - bottom;
  float d = far - near;

  Mat44 result = Mat44::identity();
  result.a = 2 / w;  // -1 <= x' <= 1
  result.f = 2 / h;  // -1 <= y' <= 1
  result.k = -1 / d; // 0 <= z' <= 1

  result.m = -(right + left) / w;
  result.n = -(top + bottom) / h;
  result.o = -near / d;

  return result;
}

static Mat44 perspective_matrix(float near, float far, float fov)
{
  Mat44 result = Mat44::identity();
  result.k = (-far / (far - near));
  result.o = (-(far * near) / (far - near));
  result.l = -1.0f;
  result.p = 0.0f;

  float s = 1.0f / (tan(fov / 2.0f * PI / 180.0f));
  result.a = s;
  result.f = s;

  return result;
}

static Mat44 viewport_matrix(float width, float height, bool fix_aspect = false)
{
  Mat44 result = Mat44::identity();

  float hw = width / 2.0f;
  float hh = height / 2.0f;

  if (fix_aspect) {
    if (hw > hh) {
      result.a = hw / (hw / hh);
      result.f = hh;
    } else {
      result.a = hw;
      result.f = hh / (hh / hw);
    }
  } else {
    result.a = hw;
    result.f = hh;
  }

  result.m = hw;
  result.n = hh;

  return result;
}

Mat44 look_at_matrix(Vec3f from, Vec3f to, Vec3f up)
{
  Mat44 result = Mat44::identity();
  Mat44 translate = Mat44::translate(-from.x, -from.y, -from.z);

  Vec3f bz = (from - to).normalized();
  Vec3f bx = up.cross(bz).normalized();
  Vec3f by = bz.cross(bx).normalized();

  result.a = bx.x;
  result.e = bx.y;
  result.i = bx.z;
  result.b = by.x;
  result.f = by.y;
  result.j = by.z;
  result.c = bz.x;
  result.g = bz.y;
  result.k = bz.z;
  result = translate * result;

  return result;
}
