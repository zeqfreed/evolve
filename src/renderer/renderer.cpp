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

inline static int32_t edge_func(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  return (x0 * y1) - (x1 * y0);
}

static void draw_triangle(RenderingContext *ctx, IShader *shader, bool only_z = false)
{
#define BLOCK_SIZE 8
#define IROUND(v) ((int32_t) (v + 0.5))

  Vec3f p0 = shader->positions[0] * ctx->viewport_mat;
  Vec3f p1 = shader->positions[1] * ctx->viewport_mat;
  Vec3f p2 = shader->positions[2] * ctx->viewport_mat;

  int32_t px[3] = {IROUND(p0.x), IROUND(p1.x), IROUND(p2.x)};
  int32_t py[3] = {IROUND(p0.y), IROUND(p1.y), IROUND(p2.y)};

  int32_t minx = MIN3(px[0], px[1], px[2]);
  int32_t miny = MIN3(py[0], py[1], py[2]);
  int32_t maxx = MAX3(px[0], px[1], px[2]);
  int32_t maxy = MAX3(py[0], py[1], py[2]);

  int32_t target_width = ctx->target->width;
  int32_t target_height = ctx->target->height;

  if (maxx < 0 || maxy < 0 ||
      minx >= target_width || miny >= target_height) {
    return;
  }

  // Clip bounding rect to target rect
  minx = MAX(0, minx);
  miny = MAX(0, miny);
  maxx = MIN(maxx, target_width - 1);
  maxy = MIN(maxy, target_height - 1);

  float area = edge_func(px[1] - px[0], py[1] - py[0], px[2] - px[1], py[2] - py[1]);
  if (area <= 0) {
    return;
  }

  float rarea = 1.0 / area;

  float dz1 = p1.z - p0.z;
  float dz2 = p2.z - p0.z;
  
  Vec3f w_xinc = {(py[1] - py[2]) * rarea,
                  (py[2] - py[0]) * rarea,
                  (py[0] - py[1]) * rarea};
  Vec3f w_yinc = {(px[2] - px[1]) * rarea,
                  (px[0] - px[2]) * rarea,
                  (px[1] - px[0]) * rarea};

  int blkminx = minx & ~(BLOCK_SIZE - 1);
  int blkminy = miny & ~(BLOCK_SIZE - 1);

  int blkmaxx = (maxx + BLOCK_SIZE) & ~(BLOCK_SIZE - 1);
  if (blkmaxx > target_width) blkmaxx -= BLOCK_SIZE;

  int blkmaxy = (maxy + BLOCK_SIZE) & ~(BLOCK_SIZE - 1);
  if (blkmaxy > target_height) blkmaxy -= BLOCK_SIZE;

  int blkcountx = (blkmaxx - blkminx) / BLOCK_SIZE;
  int blkcounty = (blkmaxy - blkminy) / BLOCK_SIZE;

  Vec3f blk_xinc = w_xinc * BLOCK_SIZE;
  Vec3f blk_yinc = w_yinc * BLOCK_SIZE; 

  Vec3f c = {((px[1] * py[2]) - (py[1] * px[2])) * rarea,
             ((px[2] * py[0]) - (py[2] * px[0])) * rarea,
             ((px[0] * py[1]) - (py[0] * px[1])) * rarea};

  Vec3f basew = c + w_xinc * blkminx + w_yinc * blkminy;

  int blockX = 0;
  int blockY = 0;

#define INSIDE_TRIANGLE(w) (w.x >= 0.0 && w.y >= 0.0 && w.z >= 0.0)
#define INOUT(w) (((w.x >= 0) << 0) | ((w.y >= 0) << 1) | ((w.z >= 0) << 2))

  for (; blockY < blkcounty; blockY++) {
    blockX = 0;

    Vec3f blockW[4];
    blockW[1] = basew + blk_xinc * blockX + blk_yinc * blockY;
    blockW[3] = blockW[1] + blk_yinc;
    
    int inout[4] = {0, 0, 0, 0};
    inout[1] = INOUT(blockW[1]);
    inout[3] = INOUT(blockW[3]);

    for (; blockX < blkcountx; blockX++) {
      blockW[0] = blockW[1];
      blockW[2] = blockW[3];
      blockW[1] = blockW[0] + blk_xinc;
      blockW[3] = blockW[2] + blk_xinc;

      //blockW[0] = basew + blk_xinc * blockX + blk_yinc * blockY;
      //blockW[1] = blockW[0] + blk_xinc;
      //blockW[2] = blockW[0] + blk_yinc;
      //blockW[3] = blockW[2] + blk_xinc;

      inout[0] = inout[1];
      inout[2] = inout[3];
      inout[1] = INOUT(blockW[1]);
      inout[3] = INOUT(blockW[3]);

      //inside[0] = INSIDE_TRIANGLE(blockW[0]);
      //inside[1] = INSIDE_TRIANGLE(blockW[1]);
      //inside[2] = INSIDE_TRIANGLE(blockW[2]);
      //inside[3] = INSIDE_TRIANGLE(blockW[3]);

      bool allSame = (inout[0] == inout[1]) && (inout[0] == inout[2]) && (inout[0] == inout[3]);
      bool allInside = allSame && (inout[0] == 7);
      bool allOutside = allSame && !allInside;

      if (allOutside) {
        // Block is outside of the triangle
        continue;

      } else if (allInside) {
        // Block is fully inside the triangle

        int bx = blockX * BLOCK_SIZE;
        int by = blockY * BLOCK_SIZE;

        Vec3f wrow = blockW[0];

        for (int j = blkminy + by; j < blkminy + by + BLOCK_SIZE; j++) {
          Vec3f w = wrow;

          for (int i = blkminx + bx; i < blkminx + bx + BLOCK_SIZE; i++) {
            zval_t zvalue = (1 - (p0.z + w.y * dz1 + w.z * dz2)) * ZBUFFER_MAX;
            int zoffset = j * target_width + i;
            if (zvalue >= ctx->zbuffer[zoffset]) {
              ctx->zbuffer[zoffset] = zvalue;
              Vec3f color;
              if (!only_z && shader->fragment(ctx, w.x, w.y, w.z, &color)) {
                set_pixel(ctx->target, i, j, color);
              }
            }

            w = w + w_xinc;
          }

          wrow = wrow + w_yinc;
        }
      } else {
        // Block is partially inside the triangle
        
        Vec3f wrow = blockW[0];
      
        int bx = blockX * BLOCK_SIZE;
        int by = blockY * BLOCK_SIZE;
        for (int j = blkminy + by; j < blkminy + by + BLOCK_SIZE; j++) {
          Vec3f w = wrow;

          for (int i = blkminx + bx; i < blkminx + bx + BLOCK_SIZE; i++) {
            if (INSIDE_TRIANGLE(w)) {
              zval_t zvalue = (1 - (p0.z + w.y * dz1 + w.z * dz2)) * ZBUFFER_MAX;
              int zoffset = j * target_width + i;
              if (zvalue >= ctx->zbuffer[zoffset]) {
                ctx->zbuffer[zoffset] = zvalue;
                Vec3f color;
                if (!only_z && shader->fragment(ctx, w.x, w.y, w.z, &color)) {
                  set_pixel(ctx->target, i, j, color);
                }
              }
            }

            w = w + w_xinc;
          }

          wrow = wrow + w_yinc;
        }
      }
    }
  }

#undef INOUT
#undef IROUND
#undef INSIDE_TRIANGLE
#undef BLOCK_SIZE
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
