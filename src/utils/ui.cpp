#include "ui.h"

typedef struct UIShaderData {
  Vec3f pos[3];
  Vec4f color;
} UIShaderData;

FRAGMENT_FUNC(fragment_ui)
{
  UIShaderData *d = (UIShaderData *) shader_data;
  *color = d->color;
  return true;
}


static void ui_init(UIContext *ctx, RenderingContext *renderingContext, Font *font,
                    KeyboardState *keyboardState, MouseState *mouseState)
{
  ASSERT(ctx);

  ctx->font = font;
  ctx->keyboardState = keyboardState;
  ctx->mouseState = mouseState;
  ctx->renderingContext = renderingContext;

  ctx->x = 100.0f;
  ctx->y = 100.0f;
  ctx->spacing = 0.0f;
}

static void ui_rect(UIContext *ctx, float x0, float y0, float x1, float y1, Vec4f color)
{

  UIShaderData data = {};
  data.color = color;

  float x = ctx->x;
  float y = ctx->y;

  Mat44 mvp = ctx->renderingContext->mvp_mat;

  Vec3f pos[4] = {
    Vec3f{x0 + x, y0 + y, 0.0f} * mvp,
    Vec3f{x1 + x, y0 + y, 0.0f} * mvp,
    Vec3f{x1 + x, y1 + y, 0.0f} * mvp,
    Vec3f{x0 + x, y1 + y, 0.0f} * mvp
  };

  RenderingContext *rctx = ctx->renderingContext;
  rctx->draw_triangle(ctx->renderingContext, &fragment_ui, (void *) &data, pos[0], pos[1], pos[2]);
  rctx->draw_triangle(ctx->renderingContext, &fragment_ui, (void *) &data, pos[0], pos[2], pos[3]);
}

void ui_button(UIContext *ctx, float width, float height, uint8_t *text)
{
  ui_rect(ctx, 0, 0, width, height, Vec4f{0.55f, 0.55f, 0.55f, 1.0f});

  float textWidth = font_get_text_width(ctx->font, text);
  float xOffset = 0.5 * (width - textWidth);
  float yOffset = height - 0.5f * ctx->font->capHeight;

  font_render_text(ctx->font, ctx->renderingContext, ctx->x + xOffset, ctx->y + yOffset, text);
}
