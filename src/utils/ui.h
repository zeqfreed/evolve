#pragma once

typedef struct UIContext {
  KeyboardState *keyboardState;
  MouseState *mouseState;
  RenderingContext *renderingContext;
  Font *font;
  float x;
  float y;
  float spacing;
  uint32_t active_hash;
} UIContext;

typedef struct UIRect {
  float x0;
  float y0;
  float x1;
  float y1;
} UIRect;
