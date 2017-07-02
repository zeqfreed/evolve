#include "font.h"
#include "renderer.h"

#define FONT_FIRST_CHAR '!'

typedef struct FontShaderData {
  Vec3f pos[3];
  Vec3f uv0;
  Vec3f duv[2];
  int clampu;
  int clampv;
  Texture *texture;
} FontShaderData;

FRAGMENT_FUNC(fragment_text)
{
  FontShaderData *d = (FontShaderData *) shader_data;

  float fu = d->uv0.x + t1 * d->duv[0].x + t2 * d->duv[1].x;
  float fv = d->uv0.y + t1 * d->duv[0].y + t2 * d->duv[1].y;  

  int u = ((int) fu) & d->clampu;
  int v = ((int) fv) & d->clampv;
  Vec3f tcolor = d->texture->pixels[v * d->texture->width + u];

  *color = {tcolor.r, tcolor.g, tcolor.b};

  return tcolor.r;
}

void Font::render_string(RenderingContext *ctx, float x, float y, const char *string)
{
  FontShaderData data = {};
  data.texture = texture;
  data.clampu = texture->width - 1;
  data.clampv = texture->height - 1;

  const char *p = string;
  float xoffset = 0;

  while (*p) {
    if (*p == ' ') {
      xoffset += spec.char_width;
      p++;
      continue;
    }

    uint32_t chidx = *p - FONT_FIRST_CHAR;
    uint32_t chx = chidx % spec.chars_per_line;
    uint32_t chy = chidx / spec.chars_per_line;

    Vec3f texture_coords[4] = {};
    texture_coords[0] = {(float) (chx * spec.char_width), (float) (texture->height - chy * spec.char_height), 0};
    texture_coords[1] = texture_coords[0] + Vec3f::make(0.0f, (float) -spec.char_height, 0.0f);
    texture_coords[2] = texture_coords[0] + Vec3f::make((float) spec.char_width - 1.0f, (float) -spec.char_height, 0.0f);
    texture_coords[3] = texture_coords[0] + Vec3f::make((float) spec.char_width - 1.0f, 0.0f, 0.0f);

    Vec3f positions[4] = {};
    positions[0] = {xoffset + x, y, 0};
    positions[1] = positions[0] +  Vec3f::make(0.0f, (float) spec.char_height, 0.0f);
    positions[2] = positions[0] +  Vec3f::make((float) spec.char_width, (float) spec.char_height, 0.0f);
    positions[3] = positions[0] +  Vec3f::make((float) spec.char_width, 0.0f, 0.0f);

    for (int i = 0; i < 4; i++) {
      positions[i] = positions[i] * ctx->mvp_mat;
    }

    data.uv0 = texture_coords[0];
    data.duv[0] = texture_coords[1] - data.uv0;
    data.duv[1] = texture_coords[2] - data.uv0;
    draw_triangle(ctx, &fragment_text, (void *) &data, positions[0], positions[1], positions[2]);

    data.duv[0] = texture_coords[2] - data.uv0;
    data.duv[1] = texture_coords[3] - data.uv0;
    draw_triangle(ctx, &fragment_text, (void *) &data, positions[0], positions[2], positions[3]);

    xoffset += spec.char_width;
    p++;
  }
}
