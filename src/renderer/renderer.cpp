#include <cstdlib>
#include "renderer.h"

static inline void set_pixel(Texture *target, int32_t x, int32_t y, Vec3f color)
{
  target->pixels[y * target->width + x] = color;
}

static inline void set_pixel_safe(Texture *target, uint32_t x, uint32_t y, Vec3f color)
{
  if (x >= target->width) { return; }
  if (y >= target->height) { return; }

  set_pixel(target, x, y, color);
}

static void draw_line(Texture *target, int32_t x0, int32_t y0, int32_t x1, int32_t y1, Vec3f color)
{
  x0 = CLAMP(x0, 0, target->width - 1);
  x1 = CLAMP(x1, 0, target->height - 1);
  y0 = CLAMP(y0, 0, target->width - 1);
  y1 = CLAMP(y1, 0, target->height - 1);

  int32_t t;
  bool transposed = abs(y1-y0) > abs(x1-x0);
  if (transposed) {
    t = x0;
    x0 = y0;
    y0 = t;

    t = x1;
    x1 = y1;
    y1 = t;
  }

  if (x0 > x1) {
    t = x0;
    x0 = x1;
    x1 = t;

    t = y0;
    y0 = y1;
    y1 = t;
  }

  int32_t dx = x1 - x0;
  int32_t dy = y1 - y0;

  float eps = 0.0000001f;
  if (dx < eps && dy < eps) {
    return; // Degenerate case
  }

  int32_t y = y0;
  int32_t slope = 2 * abs(dy);
  int32_t error = 0;
  int32_t inc = dy > 0 ? 1 : -1;

  for (int32_t x = x0; x <= x1; x++) {
    if (transposed) {
      set_pixel_safe(target, y, x, color);
    } else {
      set_pixel_safe(target, x, y, color);
    }

    error += slope;
    if (error > dx) {
      y += inc;
      error -= 2*dx;
    }
  }
}

static inline Texel blend(Texel src, Texel dst)
{
#if 1
  if (src.a == 1.0) {
    return src;
  }
  
  Vec3f c = src.rgb * src.a + dst.rgb * (1.0f - src.a);
  return (Texel) Vec4f(c, 1.0f);
#else
  return src;
#endif  
}

static inline uint32_t blend(Texel src, uint32_t dst)
{
#if 0
  if (src.a == 1.0) {
    return src;
  }
  
  Vec3f c = src.rgb * src.a + dst.rgb * (1.0f - src.a);
  return (Texel) Vec4f(c, 1.0f);
#else
  return rgba_color(src);
#endif
}

static void precalculate_matrices(RenderingContext *ctx)
{
  ctx->normal_mat = ctx->model_mat.inverse().transposed();
  ctx->modelview_mat = ctx->model_mat * ctx->view_mat;

  Mat44 mvp = ctx->modelview_mat * ctx->projection_mat;
  ctx->mvp_mat = mvp;

  Vec4f p = {mvp.c, mvp.g, mvp.k, mvp.o};
  float mag = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
  ctx->near_clip_plane = p * (1 / mag);
}

inline static q8 edge_funcq(q8 x0, q8 y0, q8 x1, q8 y1)
{
  return qmul(x0, y1) - qmul(x1, y0);
}

#define DRAW_TRIANGLE_NO_FRAGMENT
#define DRAW_TRIANGLE_TARGET_TYPE Texture
#define DRAW_TRIANGLE_TEXEL_TYPE Texel
#define DRAW_TRIANGLE_FUNC_NAME draw_triangle
#include "draw_triangle.cpp"

#define DRAW_TRIANGLE_TARGET_TYPE DrawingBuffer
#define DRAW_TRIANGLE_TEXEL_TYPE uint32_t
#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba32
#include "draw_triangle.cpp"

#include <emmintrin.h>

static inline void set_target(RenderingContext *ctx, Texture *texture)
{
  ctx->target = texture;
  ctx->target_width = texture->width;
  ctx->target_height = texture->height;
  ctx->draw_triangle = &draw_triangle;
}

static inline void set_target(RenderingContext *ctx, DrawingBuffer *buffer)
{
  ctx->target = buffer;
  ctx->target_width = buffer->width;
  ctx->target_height = buffer->height;
  ctx->draw_triangle = &draw_triangle_rgba32;
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
