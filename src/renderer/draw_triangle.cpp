
#define DRAW_TRIANGLE_TEXELP(TARGET, X, Y) (&((DRAW_TRIANGLE_TEXEL_TYPE *) TARGET->pixels)[(Y) * TARGET->width + (X)])

#ifndef DRAW_TRIANGLE_COLOR_TO_TEXEL
  #define DRAW_TRIANGLE_COLOR_TO_TEXEL(V) (V)
#endif

#ifndef DRAW_TRIANGLE_TEXEL_TO_COLOR
  #define DRAW_TRIANGLE_TEXEL_TO_COLOR(V) (V)
#endif

#if DRAW_TRIANGLE_BLEND
  #define DRAW_TRIANGLE_DO_BLEND(CTX, SRC, DST) DRAW_TRIANGLE_TEXEL_TO_COLOR(ctx->blend_func(SRC, DRAW_TRIANGLE_COLOR_TO_TEXEL(DST)))
#else
  #define DRAW_TRIANGLE_DO_BLEND(CTX, SRC, DST) DRAW_TRIANGLE_TEXEL_TO_COLOR(SRC)
#endif

#define ALPHA_TEST(A) (A > 0.0f)

static DRAW_TRIANGLE_FUNC(DRAW_TRIANGLE_FUNC_NAME)
{
#define BLOCK_SIZE 8
#define IROUND(v) (to_q8((float) (v)))

  DRAW_TRIANGLE_TARGET_TYPE *target = (DRAW_TRIANGLE_TARGET_TYPE *) ctx->target;

  p0 = p0 * ctx->viewport_mat;
  p1 = p1 * ctx->viewport_mat;
  p2 = p2 * ctx->viewport_mat;

  q8 px[3] = {IROUND(p0.x), IROUND(p1.x), IROUND(p2.x)};
  q8 py[3] = {IROUND(p0.y), IROUND(p1.y), IROUND(p2.y)};

  q8 area = edge_funcq(px[1] - px[0], py[1] - py[0], px[2] - px[1], py[2] - py[1]);

#if DRAW_TRIANGLE_CULL
  if (area <= 0) {
    return;
  }
#endif

  int32_t minx = qint(MIN3(px[0], px[1], px[2]));
  int32_t miny = qint(MIN3(py[0], py[1], py[2]));
  int32_t maxx = qint(MAX3(px[0], px[1], px[2]));
  int32_t maxy = qint(MAX3(py[0], py[1], py[2]));

  int32_t target_width = target->width;
  int32_t target_height = target->height;

  if (maxx < 0 || maxy < 0 ||
      minx >= (target_width) || miny >= (target_height)) {
    return;
  }

  // Clip bounding rect to target rect
  minx = MAX(0, minx);
  miny = MAX(0, miny);
  maxx = MIN(maxx, (target_width - 1));
  maxy = MIN(maxy, (target_height - 1));

  float rarea = 1.0f / to_float(area);

  float z0 = p0.z;
  float dz1 = p1.z - z0;
  float dz2 = p2.z - z0;

  Vec3q w_xinc = {(py[1] - py[2]),
                  (py[2] - py[0]),
                  (py[0] - py[1])}; // * rarea;
  Vec3q w_yinc = {(px[2] - px[1]),
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

  Vec3q c = {(qmul(px[1], py[2]) - qmul(py[1], px[2])),
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
#if DRAW_TRIANGLE_CULL
      bool allInside = allSame && (inout[0] == 7);
#else
      bool allInside = allSame && (inout[0] == 7 || inout[0] == 0);
#endif
      bool allOutside = allSame && !allInside;

      if (allOutside) {
        continue; /* Block is outside of the triangle */
      }

      int bx = qint(blockX) * BLOCK_SIZE;
      int by = qint(blockY) * BLOCK_SIZE;

      int starty = blkminy + by;
      //int endy = starty + BLOCK_SIZE;
      int startx = blkminx + bx;
      //int endx = blkminx + bx + BLOCK_SIZE;

      float t1row = to_float(blockW[0].y) * rarea;
      float t2row = to_float(blockW[0].z) * rarea;

      float zrow = 1 - (z0 + t1row * dz1 + to_float(blockW[0].z) * rarea * dz2);
      float zmaxx = 1 - (z0 + to_float(blockW[1].y) * rarea * dz1 + to_float(blockW[1].z) * rarea * dz2);
      float zmaxy = 1 - (z0 + to_float(blockW[2].y) * rarea * dz1 + to_float(blockW[2].z) * rarea * dz2);
      float zdx = (zmaxx - zrow) / BLOCK_SIZE;
      float zdy = (zmaxy - zrow) / BLOCK_SIZE;

      zval_t *zp_row = &ctx->zbuffer[starty * target_width + startx];
      DRAW_TRIANGLE_TEXEL_TYPE *bufferp_row = DRAW_TRIANGLE_TEXELP(target, startx, starty);

      if (allInside) {
        // Block is fully inside the triangle

        for (int j = 0; j < BLOCK_SIZE; j++) {
          float t1 = t1row;
          float t2 = t2row;
          float z = zrow;

          zval_t *zp = zp_row;
          DRAW_TRIANGLE_TEXEL_TYPE *bufferp = bufferp_row;

          for (int i = 0; i < BLOCK_SIZE; i++) {
            zval_t zvalue = (zval_t) (z * ZBUFFER_MAX);
            if (ZTEST(zvalue, *zp)) {
              #if DRAW_TRIANGLE_FRAG
                Texel color = {};
                if (fragment(ctx, shader_data, startx + i, starty + j, 1 - t1 - t2, t1, t2, &color)) {
                  *bufferp = DRAW_TRIANGLE_DO_BLEND(ctx, color, *bufferp);
                }

                if (ALPHA_TEST(color.a)) {
                  *zp = zvalue;
                }
              #else
                *zp = zvalue;
              #endif
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
          bufferp_row += target_width;
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
          DRAW_TRIANGLE_TEXEL_TYPE *bufferp = bufferp_row;

#if DRAW_TRIANGLE_CULL
  #define INSIDE_TRIANGLE(w) ((w.x | w.y | w.z) >= 0)
#else
  #define INSIDE_TRIANGLE(w) (((w.x | w.y | w.z) >= 0) || ((w.x < 0.0f) && (w.y < 0.0f) && (w.z < 0.0f)))
#endif

          for (int i = 0; i < BLOCK_SIZE; i++) {
            if (INSIDE_TRIANGLE(w)) {
              zval_t zvalue = (zval_t) (z * ZBUFFER_MAX);
              if (ZTEST(zvalue, *zp)) {
                #if DRAW_TRIANGLE_FRAG
                  Texel color = {};
                  if (fragment(ctx, shader_data, startx + i, starty + j, 1 - t1 - t2, t1, t2, &color)) {
                    *bufferp = DRAW_TRIANGLE_DO_BLEND(ctx, color, *bufferp);
                  }

                  if (ALPHA_TEST(color.a)) {
                    *zp = zvalue;
                  }
                #else
                  *zp = zvalue;
                #endif
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
          bufferp_row += target_width;
        }
      }
    }
  }

#undef INSIDE_TRIANGLE
#undef INOUT
#undef IROUND
#undef BLOCK_SIZE
}

#undef ALPHA_TEST

#undef DRAW_TRIANGLE_DO_BLEND
#undef DRAW_TRIANGLE_TEXELP

#undef DRAW_TRIANGLE_FUNC_NAME
#undef DRAW_TRIANGLE_TARGET_TYPE
#undef DRAW_TRIANGLE_TEXEL_TYPE

#undef DRAW_TRIANGLE_COLOR_TO_TEXEL
#undef DRAW_TRIANGLE_TEXEL_TO_COLOR

#undef DRAW_TRIANGLE_BLEND
#undef DRAW_TRIANGLE_CULL
#undef DRAW_TRIANGLE_FRAG
