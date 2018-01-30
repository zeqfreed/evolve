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
  ctx->group_hash = 0;

  ctx->x = 100.0f;
  ctx->y = 100.0f;
  ctx->spacing = 0.0f;

  ctx->layout.mode = UI_LM_NORMAL;
  ctx->layout.xspacing = 4.0f;
  ctx->layout.yspacing = 4.0f;
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

static inline uint32_t fnv_hash_add_data(uint32_t start, void *bytes, size_t size)
{
  uint32_t result = start;

  for (int i = 0; i < size; i++) {
    result ^= *((uint8_t *) bytes + i);
    result *= FNV_PRIME;
  }

  return result;
}

static inline uint32_t fnv_hash(void *bytes, size_t size)
{
  return fnv_hash_add_data(FNV_START, bytes, size);
}

static inline uint32_t fnv_hash_add_string(uint32_t current, char *string)
{
  uint32_t result = current;

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

static uint32_t ui_hash(UIContext *ctx, uint8_t type, uint8_t *label)
{
  ASSERT(ctx != NULL);

  uint32_t result;
  if (ctx->group_hash != 0) {
    result = fnv_hash_add_data(ctx->group_hash, (void *) &type, sizeof(type));
  } else {
    result = fnv_hash((void *) &type, 1);
  }

  return fnv_hash_add_string(result, (char *) label);
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

UIRect ui_layout__calc_rect(UIContext *ctx, UIRect rect);
void ui_layout__shrink_row(UIContext *ctx, UIRect rect);

ui_button_result ui_button(UIContext *ctx, float width, float height, uint8_t *text)
{
  uint32_t hash = ui_hash(ctx, UI_BUTTON_TYPE, text);

  UIRect rect = ui_layout__calc_rect(ctx, {0, 0, width, height});
  ui_layout__shrink_row(ctx, rect);

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
  float xOffset = 0.5 * (rect.x1 - rect.x0 - textWidth);
  float yOffset = height - 0.5f * ctx->font->capHeight;

  font_render_text(ctx->font, ctx->renderingContext, rect.x0 + xOffset, rect.y0 + yOffset, text);

  return result;
}

void ui_label(UIContext *ctx, float width, float height, uint8_t *text)
{
  UIRect rect = ui_layout__calc_rect(ctx, {0, 0, width, height});
  ui_layout__shrink_row(ctx, rect);

  ui_rect(ctx, rect, Vec4f(0.1f, 0.1f, 0.1f, 1.0f));

  float textWidth = font_get_text_width(ctx->font, text);
  float xOffset = 0.5 * (rect.x1 - rect.x0 - textWidth);
  float yOffset = height - 0.5f * ctx->font->capHeight;

  font_render_text(ctx->font, ctx->renderingContext, rect.x0 + xOffset, rect.y0 + yOffset, text);
}

void ui_begin(UIContext *ctx, float x, float y)
{
  ASSERT(ctx != NULL);
  ctx->x = x;
  ctx->y = y;
}

void ui_end(UIContext *ctx)
{
  return;
}

void ui_layout_row_begin(UIContext *ctx, float width, float height)
{
  ASSERT(ctx != NULL);

  if (ctx->layout.isRow) {
    return;
  }

  ctx->layout.isRow = true;
  ctx->layout.row = {ctx->x, ctx->y, ctx->x + width, ctx->y + height};
  ctx->layout.mode = UI_LM_NORMAL;

  return;
}

void ui_layout_row_end(UIContext *ctx, float spacing = 8.0f)
{
  ASSERT(ctx != NULL);

  if (!ctx->layout.isRow) {
    return;
  }

  ctx->layout.isRow = false;
  ctx->y += (ctx->layout.row.y1 - ctx->layout.row.y0 + ctx->layout.yspacing);
}

void ui_layout_pull_right(UIContext *ctx)
{
  ASSERT(ctx != NULL);
  ctx->layout.mode = UI_LM_PULL_RIGHT;
}

void ui_layout_fill(UIContext *ctx)
{
  ASSERT(ctx != NULL);
  ctx->layout.mode = UI_LM_FILL;
}

UIRect ui_layout__calc_rect(UIContext *ctx, UIRect rect)
{
  ASSERT(ctx != NULL);

  if (!ctx->layout.isRow) {
    return {rect.x0 + ctx->x, rect.y0 + ctx->y, rect.x1 + ctx->x, rect.y1 + ctx->y};
  }

  switch (ctx->layout.mode) {
    case UI_LM_NORMAL: {
      float offset = ctx->layout.row.x0 + ctx->layout.xspacing;
      return {rect.x0 + offset, rect.y0 + ctx->y, rect.x1 + offset, rect.y1 + ctx->y};
    }

    case UI_LM_PULL_RIGHT: {
      float width = rect.x1 - rect.x0;
      return {ctx->layout.row.x1 - width - ctx->layout.xspacing, rect.y0 + ctx->y, ctx->layout.row.x1 - ctx->layout.xspacing, rect.y1 + ctx->y};
    }

    case UI_LM_FILL: {
      return {ctx->layout.row.x0 + ctx->layout.xspacing, rect.y0 + ctx->y, ctx->layout.row.x1 - ctx->layout.xspacing, rect.y1 + ctx->y};
    }
  }
}

void ui_layout__shrink_row(UIContext *ctx, UIRect rect)
{
  ASSERT(ctx != NULL);

  if (!ctx->layout.isRow) {
    return;
  }

  switch (ctx->layout.mode) {
  case UI_LM_NORMAL:
    ctx->layout.row.x0 += (rect.x1 - rect.x0) + ctx->layout.xspacing;
    break;
  case UI_LM_PULL_RIGHT:
    ctx->layout.row.x1 -= (rect.x1 - rect.x0) + ctx->layout.xspacing;
    break;
  case UI_LM_FILL:
    float halfway = 0.5f * (ctx->layout.row.x0 + ctx->layout.row.x1);
    ctx->layout.row.x0 = halfway;
    ctx->layout.row.x1 = halfway;
    break;
  }
}

void ui_group_begin(UIContext *ctx, void *data, size_t size)
{
  ASSERT(ctx != NULL);
  ASSERT(ctx->group_hash == 0);

  ctx->group_hash = fnv_hash(data, size);
}

void ui_group_begin(UIContext *ctx, char *string)
{
  ctx->group_hash = fnv_hash_add_string(FNV_START, string);
}

void ui_group_end(UIContext *ctx)
{
  ASSERT(ctx != NULL);
  ASSERT(ctx->group_hash != 0);

  ctx->group_hash = 0;
}
