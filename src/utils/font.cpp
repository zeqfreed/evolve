#include "renderer/renderer.h"

#include "font.h"

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

  *color = TEXEL4F(d->texture, u, v);

  return true;
}

void font_init(Font *font, void *bytes, size_t size) {
  ASSERT(font);
  // TODO: Assert correct size

  FTDHeader *header = (FTDHeader *) bytes;
  font->fontSize = header->fontSize;
  font->lineHeight = (float) header->lineHeight;
  font->capHeight = header->capHeight;
  font->codepointsCount = header->codepointsCount;
  font->rangesCount = header->rangesCount;
  font->ranges = (uint32_t *) ((uint8_t *) bytes + header->rangesOffset);
  font->quads = (FontQuad *) ((uint8_t *) bytes + header->quadsOffset);
  font->kernPairs = (float *) ((uint8_t *) bytes + header->kernPairsOffset);
}

inline FontQuad *font_codepoint_quad(Font *font, uint32_t codepoint0, uint32_t codepoint1, float *kerning)
{
  ASSERT(font);

  uint32_t offset = 0;
  uint32_t idx0 = 0;
  uint32_t idx1 = 0;
  int found = 0;

  for (size_t i = 0; i < font->rangesCount; i++) {
    uint32_t first = font->ranges[2*i];
    uint32_t onePastLast = font->ranges[2*i+1];

    if ((codepoint0 >= first) && (codepoint0 < onePastLast)) {
      idx0 = offset + (codepoint0 - first);
      found++;
    }

    if ((codepoint1 >= first) && (codepoint1 < onePastLast)) {
      idx1 = offset + (codepoint1 - first);
      found++;
    }

    if (found == 2) {
      break;
    }

    offset += (onePastLast - first);
  }

  if (kerning) {
    *kerning = font->kernPairs[idx0*font->codepointsCount+idx1];
  }

  return &font->quads[idx0];
}

static float font_get_text_width(Font *font, uint8_t *text)
{
  ASSERT(font);

  float result = 0.0f;

  uint32_t codepoint;
  uint8_t *c = text;
  while (*c) {
    codepoint = (uint32_t) *c;
    c++;

    float kerning = 0.0f;
    FontQuad q = *font_codepoint_quad(font, codepoint, (uint32_t) *c, &kerning);

    if (codepoint == 0x20) {
      result += kerning;
      continue;
    }

    result += kerning;
  }

  return result;
}

static void font_render_text(Font *font, RenderingContext *ctx, float x, float y, uint8_t *text)
{
  ASSERT(font);

  FontShaderData data = {};
  data.texture = font->texture;
  data.clampu = data.texture->width - 1;
  data.clampv = data.texture->height - 1;

  float x0 = x;

  uint32_t codepoint;
  uint8_t *c = text;
  while (*c) {
    codepoint = (uint32_t) *c;
    c++;

    float kerning = 0.0f;
    FontQuad q = *font_codepoint_quad(font, codepoint, (uint32_t) *c, &kerning);

    if (codepoint == 0x20) {
      x += kerning;
      continue;
    }

    Vec3f tex[4] = {};
    tex[0] = {q.s0, q.t0, 0.0f};
    tex[1] = {q.s1, q.t0, 0.0f};
    tex[2] = {q.s1, q.t1, 0.0f};
    tex[3] = {q.s0, q.t1, 0.0f};

    Vec3f pos[4] = {};
    pos[0] = Vec3f{q.x0 + x, q.y0 + y, 0.0f} * ctx->mvp_mat;
    pos[1] = Vec3f{q.x1 + x, q.y0 + y, 0.0f} * ctx->mvp_mat;
    pos[2] = Vec3f{q.x1 + x, q.y1 + y, 0.0f} * ctx->mvp_mat;
    pos[3] = Vec3f{q.x0 + x, q.y1 + y, 0.0f} * ctx->mvp_mat;

    x += kerning;

    data.uv0 = tex[0];
    data.duv[0] = tex[1] - data.uv0;
    data.duv[1] = tex[2] - data.uv0;
    ctx->draw_triangle(ctx, &fragment_text, (void *) &data, pos[0], pos[1], pos[2]);

    data.duv[0] = tex[2] - data.uv0;
    data.duv[1] = tex[3] - data.uv0;
    ctx->draw_triangle(ctx, &fragment_text, (void *) &data, pos[0], pos[2], pos[3]);
  }
}
