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
  x0 = CLAMP(x0, 0, buffer->width - 1);
  x1 = CLAMP(x1, 0, buffer->height - 1);
  y0 = CLAMP(y0, 0, buffer->width - 1);
  y1 = CLAMP(y1, 0, buffer->height - 1);

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

  Vec4f p = (Vec4f){mvp.c, mvp.g, mvp.k, mvp.o};
  float mag = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
  ctx->near_clip_plane = p * (1 / mag);
}

inline static q8 edge_funcq(q8 x0, q8 y0, q8 x1, q8 y1)
{
  return qmul(x0, y1) - qmul(x1, y0);
}

static void draw_triangle(RenderingContext *ctx, FragmentFunc *fragment, void *shader_data,
                          Vec3f p0, Vec3f p1, Vec3f p2, bool only_z = false)
{
#define BLOCK_SIZE 8
#define IROUND(v) (to_q8((float) (v)))

  p0 = p0 * ctx->viewport_mat;
  p1 = p1 * ctx->viewport_mat;
  p2 = p2 * ctx->viewport_mat;

  q8 px[3] = {IROUND(p0.x), IROUND(p1.x), IROUND(p2.x)};
  q8 py[3] = {IROUND(p0.y), IROUND(p1.y), IROUND(p2.y)};

  q8 area = edge_funcq(px[1] - px[0], py[1] - py[0], px[2] - px[1], py[2] - py[1]);
  if (area <= 0) {
    return;
  }

  int32_t minx = qint(MIN3(px[0], px[1], px[2]));
  int32_t miny = qint(MIN3(py[0], py[1], py[2]));
  int32_t maxx = qint(MAX3(px[0], px[1], px[2]));
  int32_t maxy = qint(MAX3(py[0], py[1], py[2]));

  int32_t target_width = ctx->target->width;
  int32_t target_height = ctx->target->height;

  if (maxx < 0 || maxy < 0 ||
      minx >= (target_width) || miny >= (target_height)) {
    return;
  }

  // Clip bounding rect to target rect
  minx = MAX(0, minx);
  miny = MAX(0, miny);
  maxx = MIN(maxx, (target_width - 1));
  maxy = MIN(maxy, (target_height - 1));

  float rarea = 1.0 / to_float(area);

  float z0 = p0.z;
  float dz1 = p1.z - z0;
  float dz2 = p2.z - z0;
  
  Vec3q w_xinc = (Vec3q){(py[1] - py[2]),
                         (py[2] - py[0]),
                         (py[0] - py[1])}; // * rarea;
  Vec3q w_yinc = (Vec3q){(px[2] - px[1]),
                         (px[0] - px[2]),
                         (px[1] - px[0])}; // * rarea;

  int blkminx = minx & ~(BLOCK_SIZE - 1);
  int blkminy = miny & ~(BLOCK_SIZE - 1);

  // TODO: Handle edge cases or maybe use 2^n sized targets always
  int blkmaxx = (maxx + BLOCK_SIZE) & ~(BLOCK_SIZE - 1);
  if (blkmaxx > target_width) blkmaxx -= BLOCK_SIZE;

  int blkmaxy = (maxy + BLOCK_SIZE) & ~(BLOCK_SIZE - 1);
  if (blkmaxy > target_height) blkmaxy -= BLOCK_SIZE;

  int blkcountx = (blkmaxx - blkminx) / BLOCK_SIZE;
  int blkcounty = (blkmaxy - blkminy) / BLOCK_SIZE;

  Vec3q blk_xinc = w_xinc * to_q8((int32_t) BLOCK_SIZE);
  Vec3q blk_yinc = w_yinc * to_q8((int32_t) BLOCK_SIZE);

  Vec3q c = (Vec3q){(qmul(px[1], py[2]) - qmul(py[1], px[2])),
                    (qmul(px[2], py[0]) - qmul(py[2], px[0])),
                    (qmul(px[0], py[1]) - qmul(py[0], px[1]))}; // + (Vec3q){1, 1, 1};
  Vec3q basew = c + w_xinc * to_q8(blkminx) + w_yinc * to_q8(blkminy);

  float t1dx = to_float(w_xinc.y) * rarea;
  float t1dy = to_float(w_yinc.y) * rarea;
  float t2dx = to_float(w_xinc.z) * rarea;
  float t2dy = to_float(w_yinc.z) * rarea;

  q8 blockX = 0;
  q8 blockY = 0;
  q8 q_blockcntx = to_q8(blkcountx);
  q8 q_blockcnty = to_q8(blkcounty);

  for (; blockY < q_blockcnty; blockY += Q_ONE) {
    blockX = 0;

    Vec3q blockW[4];
    blockW[1] = basew + blk_yinc * blockY;
    blockW[3] = blockW[1] + blk_yinc;

#define INOUT(w) (((w.x >= 0) << 0) | ((w.y >= 0) << 1) | ((w.z >= 0) << 2))
    int inout[4] = {0, 0, 0, 0};
    inout[1] = INOUT(blockW[1]);
    inout[3] = INOUT(blockW[3]);

    for (; blockX < q_blockcntx; blockX += Q_ONE) {
      blockW[0] = blockW[1];
      blockW[2] = blockW[3];
      blockW[1] = blockW[0] + blk_xinc;
      blockW[3] = blockW[2] + blk_xinc;

      inout[0] = inout[1];
      inout[2] = inout[3];
      inout[1] = INOUT(blockW[1]);
      inout[3] = INOUT(blockW[3]);

      bool allSame = (inout[0] == inout[1]) && (inout[0] == inout[2]) && (inout[0] == inout[3]);
      bool allInside = allSame && (inout[0] == 7);
      bool allOutside = allSame && !allInside;

      if (allOutside) {
        continue; /* Block is outside of the triangle */
      }

      int bx = qint(blockX) * BLOCK_SIZE;
      int by = qint(blockY) * BLOCK_SIZE;

      int starty = blkminy + by;
      int endy = starty + BLOCK_SIZE;
      int startx = blkminx + bx;
      int endx = blkminx + bx + BLOCK_SIZE;

      float t1row = to_float(blockW[0].y) * rarea;
      float t2row = to_float(blockW[0].z) * rarea;

      float zrow = 1 - (z0 + t1row * dz1 + to_float(blockW[0].z) * rarea * dz2);
      float zmaxx = 1 - (z0 + to_float(blockW[1].y) * rarea * dz1 + to_float(blockW[1].z) * rarea * dz2);
      float zmaxy = 1 - (z0 + to_float(blockW[2].y) * rarea * dz1 + to_float(blockW[2].z) * rarea * dz2);
      float zdx = (zmaxx - zrow) / BLOCK_SIZE;
      float zdy = (zmaxy - zrow) / BLOCK_SIZE;

      zval_t *zp_row = &ctx->zbuffer[starty * target_width + startx];
      uint32_t *bufferp_row = &((uint32_t *) ctx->target->pixels)[starty * ctx->target->pitch + startx];

      if (allInside) {
        // Block is fully inside the triangle

        for (int j = 0; j < BLOCK_SIZE; j++) {
          float t1 = t1row;
          float t2 = t2row;
          float z = zrow;

          zval_t *zp = zp_row;
          uint32_t *bufferp = bufferp_row;

          for (int i = 0; i < BLOCK_SIZE; i++) {
            zval_t zvalue = z * ZBUFFER_MAX;
            if (ZTEST(zvalue, *zp)) {
              *zp = zvalue;
              Vec3f color = (Vec3f){1, 0, 1};
              if (!only_z && fragment(ctx, shader_data, startx + i, starty + j, 1 - t1 - t2, t1, t2, &color)) {
                *bufferp = rgba_color(color);
              }
            }

            t1 += t1dx;
            t2 += t2dx;
            z += zdx;
            zp++;
            bufferp++;
          }

          t1row += t1dy;
          t2row += t2dy;
          zrow += zdy;
          zp_row += target_width;
          bufferp_row += ctx->target->pitch;
        }
      } else {
        // Block is partially inside the triangle

        Vec3q wrow = blockW[0];

        for (int j = 0; j < BLOCK_SIZE; j++) {
          Vec3q w = wrow;
          float t1 = t1row;
          float t2 = t2row;
          float z = zrow;

          zval_t *zp = zp_row;
          uint32_t *bufferp = bufferp_row;

#define INSIDE_TRIANGLE(w) ((w.x | w.y | w.z) >= 0)
          for (int i = 0; i < BLOCK_SIZE; i++) {
            if (INSIDE_TRIANGLE(w)) {
              zval_t zvalue = z * ZBUFFER_MAX;
              if (ZTEST(zvalue, *zp)) {
                *zp = zvalue;
                Vec3f color = (Vec3f){0.2, 0.2, 0.2};
                if (!only_z && fragment(ctx, shader_data, startx + i, starty + j, 1 - t1 - t2, t1, t2, &color)) {
                  *bufferp = rgba_color(color);
                }
              }
            }

            t1 += t1dx;
            t2 += t2dx;
            z += zdx;
            w = w + w_xinc;
            zp++;
            bufferp++;
          }

          t1row += t1dy;
          t2row += t2dy;
          zrow += zdy;
          wrow = wrow + w_yinc;
          zp_row += target_width;
          bufferp_row += ctx->target->pitch;
        }
      }
    }
  }

#undef INSIDE_TRIANGLE
#undef INOUT
#undef IROUND
#undef BLOCK_SIZE
}

#include <emmintrin.h>

static void clear_buffer(RenderingContext *ctx)
{
  uint32_t color = rgba_color(ctx->clear_color);
  uint32_t blocks = (ctx->target->width * ctx->target->height * 32) / 128;

  __m128 value = _mm_set_epi32(color, color, color, color);
  __m128 *p = (__m128 *) ctx->target->pixels;

  while (blocks--) {
    *p++ = value;
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
  result.l = -1.0;
  result.p = 0.0;

  float s = 1.0 / (tan(fov / 2.0 * PI / 180.0));
  result.a = s;
  result.f = s;

  return result;
}

static Mat44 viewport_matrix(float width, float height, bool fix_aspect = false)
{
  Mat44 result = Mat44::identity();

  float hw = width / 2.0;
  float hh = height / 2.0;

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
