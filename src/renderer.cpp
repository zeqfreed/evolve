#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "game.h"

static void hsv_to_rgb(float h, float s, float v, float *r, float *g, float *b)
{
  h = fmod(h / 60.0, 6);
  float c = v * s;
  float x = c * (1 - fabs(fmod(h, 2) - 1));  
  float m = v - c;

  int hh = (int) h;

  *r = m;
  *g = m;
  *b = m;  
  
  if (hh == 0) {
    *r += c;
    *g += x;
  } else if (hh == 1) {
    *r += x;
    *g += c;
  } else if (hh == 2) {
    *g += c;
    *b += x;
  } else if (hh == 3) {
    *g += x;
    *b += c;
  } else if (hh == 4) {
    *r += x;
    *b += c;
  } else if (hh == 5) {
    *r += c;
    *b += x;
  }
}

void draw_frame(uint8_t *pixels, int width, int height, int pitch, int bpp)
{
  static float offset = 0.0;

  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      uint32_t *p = (uint32_t *) &pixels[(j*pitch+i)*bpp];

      float hue = (10.0 / width) * i + offset;
      if (hue > 360.0) {
        hue -= 360.0;
      }

      float h = hue;
      float s = (1.0 / height) * j;
      float v = 1.0;
      float r, g, b;

      hsv_to_rgb(h, s, v, &r, &g, &b);

      unsigned char rr = 255.0 * r;
      unsigned char gg = 255.0 * g;
      unsigned char bb = 255.0 * b;

      *p = 0xFF000000 | (bb << 16) | (gg << 8) | rr;
    }
  }

  offset += 0.25;
  if (offset > 360.0) {
    offset -= 360.0;
  }
}
