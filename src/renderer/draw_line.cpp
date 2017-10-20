#define DRAW_LINE_TEXELP(TARGET, X, Y) (&((DRAW_LINE_TEXEL_TYPE *) TARGET->pixels)[(Y) * TARGET->width + (X)])

#ifdef DRAW_LINE_TEXEL_CONVERSION_FUNC
  #define DRAW_LINE_COLOR(C) (DRAW_LINE_TEXEL_CONVERSION_FUNC(C))
#else
  #define DRAW_LINE_COLOR(C) (C)
#endif

static void draw_2d_line(DRAW_LINE_TARGET_TYPE *target, int32_t x0, int32_t y0, int32_t x1, int32_t y1, Vec4f color)
{
  if (!clip_line(&x0, &y0, &x1, &y1, 0, 0, target->width - 1, target->height - 1)) {
    return;
  }

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
      *DRAW_LINE_TEXELP(target, y, x) = DRAW_LINE_COLOR(color);
    } else {
      *DRAW_LINE_TEXELP(target, x, y) = DRAW_LINE_COLOR(color);
    }

    error += slope;
    if (error > dx) {
      y += inc;
      error -= 2.0f * dx;
    }
  }
}

static DRAW_LINE_FUNC(DRAW_LINE_FUNC_NAME)
{
  Vec4f v0 = { p0.x, p0.y, p0.z, 1.0f };
  Vec4f v1 = { p1.x, p1.y, p1.z, 1.0f };
  float d0 = v0.dot(ctx->near_clip_plane);
  float d1 = v1.dot(ctx->near_clip_plane);

  if (d0 < 0.0f && d1 < 0.0f) {
    return;
  }

  float c;
  if (d1 < 0.0f) {
    c = d0 / (d0 - d1);
    p1 = p0 + (p1 - p0) * c;
  } else if (d0 < 0.0f) {
    c = d0 / (d1 - d0);
    p0 = p0 + (p0 - p1) * c;
  }

  p0 = p0 * ctx->mvp_mat * ctx->viewport_mat;
  p1 = p1 * ctx->mvp_mat * ctx->viewport_mat;

  draw_2d_line((DRAW_LINE_TARGET_TYPE *) ctx->target, (int32_t) p0.x, (int32_t) p0.y, (int32_t) p1.x, (int32_t) p1.y, color);
}

#undef DRAW_LINE_FUNC_NAME
#undef DRAW_LINE_TARGET_TYPE
#undef DRAW_LINE_TEXEL_TYPE
#undef DRAW_LINE_TEXELP
#undef DRAW_LINE_TEXEL_CONVERSION_FUNC
#undef DRAW_LINE_COLOR
