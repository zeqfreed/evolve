#pragma once

typedef struct UIContext {
  KeyboardState *keyboardState;
  MouseState *mouseState;
  RenderingContext *renderingContext;
  Font *font;
  float x;
  float y;
  float spacing;
} UIContext;
