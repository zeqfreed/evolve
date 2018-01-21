#include <stdint.h>

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

static inline bool ui_is_hovering_rect(UIContext *ctx, UIRect r)
{
  float mx = ctx->mouseState->windowX;
  float my = ctx->mouseState->windowY;

  bool result = (mx >= r.x0 && my >= r.y0 && mx <= r.x1 && my <= r.y1);
  return result;
}

static void ui_rect(UIContext *ctx, UIRect rect, Vec4f color)
{

  UIShaderData data = {};
  data.color = color;

  Mat44 mvp = ctx->renderingContext->mvp_mat;

  Vec3f pos[4] = {
    Vec3f{rect.x0, rect.y0, 0.0f} * mvp,
    Vec3f{rect.x1, rect.y0, 0.0f} * mvp,
    Vec3f{rect.x1, rect.y1, 0.0f} * mvp,
    Vec3f{rect.x0, rect.y1, 0.0f} * mvp
  };

  RenderingContext *rctx = ctx->renderingContext;
  rctx->draw_triangle(ctx->renderingContext, &fragment_ui, (void *) &data, pos[0], pos[1], pos[2]);
  rctx->draw_triangle(ctx->renderingContext, &fragment_ui, (void *) &data, pos[0], pos[2], pos[3]);
}

#define FNV_PRIME 16777619
#define FNV_START 0x811c9dc5

static inline uint32_t fnv_hash(void *bytes, size_t size)
{
  uint32_t result = FNV_START;

  for (int i = 0; i < size; i++) {
    result ^= *((uint8_t *) bytes + i);
    result *= FNV_PRIME;
  }

  return result;
}

static inline uint32_t fnv_string_hash_add(uint32_t current, char *string)
{
  uint32_t result = FNV_START;
  while (uint8_t b = (uint8_t) *string++) {
    result ^= b;
    result *= FNV_PRIME;
  }

  return result;
}

#define UI_BUTTON_TYPE 1

#define UI_BUTTON_COLOR_NORMAL (Vec4f{0.55f, 0.55f, 0.55f, 1.0f})
#define UI_BUTTON_COLOR_HOVER (Vec4f{0.7f, 0.7f, 0.7f, 1.0f})
#define UI_BUTTON_COLOR_PRESSED (Vec4f{0.3f, 0.3f, 0.3f, 1.0f})

static uint32_t ui_hash(uint8_t type, uint8_t *label)
{
  uint32_t result = fnv_hash((void *) &type, 1);
  return fnv_string_hash_add(result, (char *) label);
}

static void ui_set_active(UIContext *ctx, uint32_t hash)
{
  ctx->active_hash = hash;
}

static bool ui_is_active(UIContext *ctx, uint32_t hash)
{
  return ctx->active_hash && ctx->active_hash == hash;
}

typedef uint32_t ui_button_result;

#define UI_BUTTON_RESULT_NONE 0
#define UI_BUTTON_RESULT_HOVERED 1
#define UI_BUTTON_RESULT_PRESSED 2
#define UI_BUTTON_RESULT_CLICKED 3

ui_button_result ui_button(UIContext *ctx, float width, float height, uint8_t *text)
{
  uint32_t hash = ui_hash(UI_BUTTON_TYPE, text);

  UIRect rect = {ctx->x, ctx->y, ctx->x + width, ctx->y + height};

  bool hover = ui_is_hovering_rect(ctx, rect);
  bool mouseWasPressed = MOUSE_BUTTON_WAS_PRESSED(ctx->mouseState, MB_LEFT);
  bool mouseIsDown = MOUSE_BUTTON_IS_DOWN(ctx->mouseState, MB_LEFT);
  bool active = ui_is_active(ctx, hash);
  bool clicked = false;

  if (mouseWasPressed && hover) {
    ui_set_active(ctx, hash);
  }

  if (!mouseIsDown && active) {
    ui_set_active(ctx, 0);
    clicked = hover;
  }

  Vec4f color = UI_BUTTON_COLOR_NORMAL;
  ui_button_result result = UI_BUTTON_RESULT_NONE;

  if (active) {
    color = UI_BUTTON_COLOR_PRESSED;
    if (mouseIsDown) {
      result = UI_BUTTON_RESULT_PRESSED;
    } else {
      result = hover ? UI_BUTTON_RESULT_CLICKED : UI_BUTTON_RESULT_NONE;
    }
  } else if (hover) {
    color = UI_BUTTON_COLOR_HOVER;
    result = UI_BUTTON_RESULT_HOVERED;
  }

  ui_rect(ctx, rect, color);

  float textWidth = font_get_text_width(ctx->font, text);
  float xOffset = 0.5 * (width - textWidth);
  float yOffset = height - 0.5f * ctx->font->capHeight;

  font_render_text(ctx->font, ctx->renderingContext, rect.x0 + xOffset, rect.y0 + yOffset, text);

  return result;
}
