#include "renderer.h"

static inline uint32_t rgba_color(Vec3f color)
{
  uint8_t r = (uint8_t) (255.0 * color.r);
  uint8_t g = (uint8_t) (255.0 * color.g);
  uint8_t b = (uint8_t) (255.0 * color.b);
  return (0xFF000000 | (b << 16) | (g << 8) | r);
}

static inline void set_pixel(DrawingBuffer *buffer, int32_t x, int32_t y, Vec3f color)
{
  int idx = (y * buffer->pitch + x) * buffer->bits_per_pixel;
  *((uint32_t *) &((uint8_t *)buffer->pixels)[idx]) = rgba_color(color);
}

static inline void set_pixel(DrawingBuffer *buffer, int32_t x, int32_t y, uint32_t rgba)
{
  int idx = (y * buffer->pitch + x) * buffer->bits_per_pixel;
  *((uint32_t *) &((uint8_t *)buffer->pixels)[idx]) = rgba;
}

static inline void set_pixel_safe(DrawingBuffer *buffer, int32_t x, int32_t y, Vec3f color)
{
  if (x < 0) { return; }
  if (y < 0) { return; }
  if (x >= buffer->width) { return; }
  if (y >= buffer->height) { return; }

  set_pixel(buffer, x, y, color);
}

static inline int32_t abs(int32_t v)
{
  return v > 0.0 ? v : -v;
}

static void draw_line(DrawingBuffer *buffer, int32_t x0, int32_t y0, int32_t x1, int32_t y1, Vec3f color)
{
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

  float eps = 0.0000001;
  if (dx < eps && dy < eps) {
    return; // Degenerate case
  }

  int32_t y = y0;
  int32_t slope = 2 * abs(dy);
  int32_t error = 0;
  int32_t inc = dy > 0 ? 1 : -1;

  for (int32_t x = x0; x <= x1; x++) {
    if (transposed) {
      set_pixel_safe(buffer, y, x, color);
    } else {
      set_pixel_safe(buffer, x, y, color);
    }

    error += slope;
    if (error > dx) {
      y += inc;
      error -= 2*dx;
    }
  }
}

static void precalculate_matrices(RenderingContext *ctx)
{
  ctx->normal_mat = ctx->model_mat.inverse().transposed();
  ctx->modelview_mat = ctx->model_mat * ctx->view_mat;

  Mat44 mvp = ctx->modelview_mat * ctx->projection_mat;
  ctx->mvp_mat = mvp;

  ctx->shadow_mat = ctx->mvp_mat.inverse() * ctx->shadow_mvp_mat;
  Vec4f p = (Vec4f){mvp.c, mvp.g, mvp.k, mvp.o};
  float mag = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
  ctx->near_clip_plane = p * (1 / mag);
}

inline static float edge_func(float x0, float y0, float x1, float y1)
{
  return (x0 * y1) - (x1 * y0);
}

static void draw_triangle(RenderingContext *ctx, IShader *shader, bool only_z = false)
{
  Vec3f p0 = shader->positions[0] * ctx->viewport_mat;
  Vec3f p1 = shader->positions[1] * ctx->viewport_mat;
  Vec3f p2 = shader->positions[2] * ctx->viewport_mat;

  int minx = MIN3(p0.x, p1.x, p2.x);
  int miny = MIN3(p0.y, p1.y, p2.y);
  int maxx = MAX3(p0.x, p1.x, p2.x);
  int maxy = MAX3(p0.y, p1.y, p2.y);

  int target_width = ctx->target->width;
  int target_height = ctx->target->height;

  if (maxx < 0 || maxy < 0 ||
      minx >= target_width || miny >= target_height) {
    return;
  }

  // Clip bounding rect to target rect
  minx = MAX(0, minx);
  miny = MAX(0, miny);
  maxx = MIN(maxx, target_width - 1);
  maxy = MIN(maxy, target_height - 1);

  float area = edge_func(p1.x - p0.x, p1.y - p0.y, p2.x - p1.x, p2.y - p1.y);
  if (area <= 0) {
    return;
  }

  float rarea = 1.0 / area;

  float dz1 = p1.z - p0.z;
  float dz2 = p2.z - p0.z;
  
  float w0_xinc = (p1.y - p2.y) * rarea;
  float w0_yinc = (p2.x - p1.x) * rarea;
  float w0_row = ((p1.x * p2.y) - (p1.y * p2.x)) * rarea + (minx + 0.5) * w0_xinc + (miny + 0.5) * w0_yinc;

  float w1_xinc = (p2.y - p0.y) * rarea;
  float w1_yinc = (p0.x - p2.x) * rarea;
  float w1_row = ((p2.x * p0.y) - (p2.y * p0.x)) * rarea + (minx + 0.5) * w1_xinc + (miny + 0.5) * w1_yinc;

  for (int j = miny; j <= maxy; j++) {
    bool inside = false;
    int joffset = j * target_width;

    float w0 = w0_row;
    float w1 = w1_row;

    for (int i = minx; i <= maxx; i++) {
      float tx = i + 0.5;
      float ty = j + 0.5;

      float w2 = 1 - w1 - w0; // Temporary hack to avoid invalid interpolated values in fragment shader

      if (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0) {
        inside = true;

        zval_t zvalue = (1 - (p0.z + w1 * dz1 + w2 * dz2)) * ZBUFFER_MAX;
        int zoffset = joffset + i;
        if (zvalue >= ctx->zbuffer[zoffset]) {
          ctx->zbuffer[zoffset] = zvalue;

          Vec3f color;
          if (!only_z && shader->fragment(ctx, w0, w1, w2, &color)) {
            set_pixel(ctx->target, i, j, color);
          }
        }
      } else {
        if (inside) {
          break;
        }
      }

      w0 += w0_xinc;
      w1 += w1_xinc;
    }

    w0_row += w0_yinc;
    w1_row += w1_yinc;
  }
}

static void clear_buffer(RenderingContext *ctx)
{
  int width = ctx->target->width;
  int height = ctx->target->height;
  uint32_t color = rgba_color(ctx->clear_color);

  for (int j = 0; j < ctx->target->height; j++) {
    for (int i = 0; i < ctx->target->width; i++) {
      set_pixel(ctx->target, i, j, color);
    }
  }
}

static void clear_zbuffer(RenderingContext *ctx)
{
  int width = ctx->target->width;
  int height = ctx->target->height;
  memset(ctx->zbuffer, ZBUFFER_MIN, width*height*sizeof(zval_t));
}

static Mat44 orthographic_matrix(float near, float far, float left, float bottom, float right, float top)
{
  float w = right - left;
  float h = top - bottom;
  float z = -1.0;
  
  Mat44 result = Mat44::identity();
  result.a = 2 / w;
  result.f = 2 / h;
  result.k = (z - near) / (far - near);

  result.m = -(right + left) / w;
  result.n = -(top + bottom) / h;

  return result;
}

static Mat44 perspective_matrix(float near, float far, float fov)
{
  Mat44 result = Mat44::identity();
  result.k = (-far / (far - near));
  result.o = (-(far * near) / (far - near));
  result.l = -1.0;
  result.p = 0.0;

  float s = 1.0 / (tan(fov / 2.0 * PI / 180.0));
  result.a = s;
  result.f = s;

  return result;
}

static Mat44 viewport_matrix(float width, float height)
{
  Mat44 result = Mat44::identity();

  float hw = width / 2.0;
  float hh = height / 2.0;

  result.a = hw / (hw / hh);
  result.m = hw;
  result.f = hh;
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
