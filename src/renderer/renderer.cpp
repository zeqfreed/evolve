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
  return Vec4f(c, 1.0f);
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
#define DRAW_TRIANGLE_FRAG 1
#include "draw_triangle.cpp"

#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba32_noblend_nocull_frag
#define DRAW_TRIANGLE_TARGET_TYPE DrawingBuffer
#define DRAW_TRIANGLE_TEXEL_TYPE uint32_t
#define DRAW_TRIANGLE_COLOR_TO_TEXEL(V) (color_rgba(V))
#define DRAW_TRIANGLE_TEXEL_TO_COLOR(V) (rgba_color(V))
#define DRAW_TRIANGLE_BLEND 0
#define DRAW_TRIANGLE_CULL 0
#define DRAW_TRIANGLE_FRAG 1
#include "draw_triangle.cpp"

#define DRAW_TRIANGLE_FUNC_NAME draw_triangle_rgba32_noblend_cull_frag
#define DRAW_TRIANGLE_TARGET_TYPE DrawingBuffer
#define DRAW_TRIANGLE_TEXEL_TYPE uint32_t
#define DRAW_TRIANGLE_COLOR_TO_TEXEL(V) (color_rgba(V))
#define DRAW_TRIANGLE_TEXEL_TO_COLOR(V) (rgba_color(V))
#define DRAW_TRIANGLE_BLEND 0
#define DRAW_TRIANGLE_CULL 1
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
        ctx->draw_triangle = &draw_triangle_rgba32_blend_nocull_frag;
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
