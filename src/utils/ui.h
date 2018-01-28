#pragma once

typedef struct UIRect {
  float x0;
  float y0;
  float x1;
  float y1;
} UIRect;

typedef enum UILayoutMode {
  UI_LM_NORMAL,
  UI_LM_PULL_RIGHT,
  UI_LM_FILL
} UILayoutMode;

typedef struct UILayout {
  UILayoutMode mode;
  bool isRow;
  UIRect row;
  float xspacing;
  float yspacing;
} UILayout;

typedef struct UIContext {
  KeyboardState *keyboardState;
  MouseState *mouseState;
  RenderingContext *renderingContext;
  UILayout layout;
  Font *font;
  float x;
  float y;
  float spacing;
  uint32_t active_hash;
} UIContext;
