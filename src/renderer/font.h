#pragma once

#include "renderer.h"

typedef struct FontSpec {
  uint8_t char_width;
  uint8_t char_height;
  uint8_t chars_per_line;
} FontSpec;

typedef struct Font {
  Texture *texture;
  FontSpec spec;

  void render_string(RenderingContext *ctx, float x, float y, const char *string);
} Font;
