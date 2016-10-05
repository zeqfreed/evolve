#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "game.h"

// static void hsv_to_rgb(float h, float s, float v, float *r, float *g, float *b)
// {
//   h = fmod(h / 60.0, 6);
//   float c = v * s;
//   float x = c * (1 - fabs(fmod(h, 2) - 1));  
//   float m = v - c;

//   int hh = (int) h;

//   *r = m;
//   *g = m;
//   *b = m;  
  
//   if (hh == 0) {
//     *r += c;
//     *g += x;
//   } else if (hh == 1) {
//     *r += x;
//     *g += c;
//   } else if (hh == 2) {
//     *g += c;
//     *b += x;
//   } else if (hh == 3) {
//     *g += x;
//     *b += c;
//   } else if (hh == 4) {
//     *r += x;
//     *b += c;
//   } else if (hh == 5) {
//     *r += c;
//     *b += x;
//   }
// }

static inline void set_pixel(DrawingBuffer *buffer, int32_t x, int32_t y, uint32_t color)
{
  if ((x < 0) || (y < 0) || (x > buffer->width) || (y > buffer->height)) {
    return;
  }

  int idx = (y * buffer->pitch + x) * buffer->bits_per_pixel;
  *((uint32_t *) &((uint8_t *)buffer->pixels)[idx]) = color;
}

static inline int32_t abs(int32_t v)
{
  return v > 0.0 ? v : -v;
}

static void draw_line(DrawingBuffer *buffer, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
  int32_t t;
  bool transposed = abs(y1-y0) > abs(x1-x0);
  if (transposed) {
    t = x0;
    x0 = y0;
    y0 = t;

    t = x1;
    x1 = y1;
    y1 = t;
  }

  if (x0 > x1) {
    t = x0;
    x0 = x1;
    x1 = t;

    t = y0;
    y0 = y1;
    y1 = t;
  }

  int32_t dx = x1 - x0;
  int32_t dy = y1 - y0;

  int32_t y = y0;
  int32_t slope = 2 * abs(dy);
  int32_t error = 0;
  int32_t inc = dy > 0 ? 1 : -1;

  for (int32_t x = x0; x <= x1; x++) {
    if (transposed) {
      set_pixel(buffer, y, x, color);
    } else {
      set_pixel(buffer, x, y, color);
    }

    error += slope;
    if (error > dx) {
      y += inc;
      error -= 2*dx;
    }
  }
}

typedef struct Vertex {
  float x;
  float y;
  float z;
  float r;
  float g;
  float b;
  float nx;
  float ny;
  float nz;
} Vertex;

inline static float edge_func(float x0, float y0, float x1, float y1)
{
  return (x0 * y1) - (x1 * y0);
}

static void draw_triangle(DrawingBuffer *buffer, Vertex *v0, Vertex *v1, Vertex *v2)
{
  int minx = v0->x;
  if (v1->x < minx) { minx = v1->x; }
  if (v2->x < minx) { minx = v2->x; }

  int miny = v0->y;
  if (v1->y < miny) { miny = v1->y; }
  if (v2->y < miny) { miny = v2->y; }  

  int maxx = v0->x;
  if (v1->x > maxx) { maxx = v1->x; }
  if (v2->x > maxx) { maxx = v2->x; }  

  int maxy = v0->y;
  if (v1->y > maxy) { maxy = v1->y; }
  if (v2->y > maxy) { maxy = v2->y; }

  for (int j = miny; j <= maxy; j++) {
    for (int i = minx; i <= maxx; i++) {
      float tx = i + 0.5;
      float ty = j + 0.5;

      float area = edge_func(v1->x - v0->x, v1->y - v0->y, v2->x - v1->x, v2->y - v1->y);
      float t0 = edge_func(v2->x - v1->x, v2->y - v1->y, tx - v1->x, ty - v1->y);
      float t1 = edge_func(v0->x - v2->x, v0->y - v2->y, tx - v2->x, ty - v2->y);
      float t2 = edge_func(v1->x - v0->x, v1->y - v0->y, tx - v0->x, ty - v0->y);
      if (t0 >= 0.0 && t1 >= 0.0 && t2 >= 0.0) {
        t0 /= area;
        t1 /= area;
        t2 /= area;
        uint8_t r = 255.0 * (v0->r * t0 + v1->r * t1 + v2->r * t2);
        uint8_t g = 255.0 * (v0->g * t0 + v1->g * t1 + v2->g * t2);
        uint8_t b = 255.0 * (v0->b * t0 + v1->b * t1 + v2->b * t2);
        set_pixel(buffer, i, j, 0xFF000000 | (r << 16) | (g << 8) | b);
      }
    }
  }
}

static int32_t tx0 = 50;
static int32_t tx1 = 400;
static int32_t tx2 = 750;
static int32_t ty0 = 50;
static int32_t ty1 = 0;
static int32_t ty2 = 0;
static int32_t tv0 = 2.0;
static int32_t tv1 = 4.0;
static int32_t tv2 = 3.0;

static bool initialized = false;

typedef struct Model {
  int vcount;
  int tcount;
  int ncount;
  int fcount;
  float vertices[2048][3];
  float texture_coords[2048][3];
  float normals[2048][3];
  int faces[4000][9];
} Model;

static Model model;

static void load_model(GlobalState *state, Model *model)
{
  FileContents contents = state->platform_api.read_file_contents((char *) "data/boar.obj");
  if (contents.size <= 0) {
    printf("Failed to load model\n");
    return;
  }

  printf("Loaded %d bytes\n", contents.size);

  int vi = 0;
  int fi = 0;

  float minx = 0;
  float maxx = 0;
  float miny = 0;
  float maxy = 0;
  float minz = 0;
  float maxz = 0;  

  char *p = (char *) contents.bytes;
  while(p < (char *) contents.bytes + contents.size) {

    float x;
    float y;
    float z;

    if (*p == 'v') {
      p++;
      if (*p == ' ') {
        int consumed = 0;
        sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed);
        p += consumed;

        model->vertices[vi][0] = x;
        model->vertices[vi][1] = y;
        model->vertices[vi][2] = z;
        vi++;

        if (x < minx) {
          minx = x;
        } else if (x > maxx) {
          maxx = x;
        }

        if (y < miny) {
          miny = y;
        } else if (y > maxy) {
          maxy = y;
        }        

        if (z < minz) {
          minz = z;
        } else if (z > maxz) {
          maxz = z;
        }        
      }
    } else if (*p == 'f') {
      p++;

      int vi0, ti0, ni0, vi1, ti1, ni1, vi2, ti2, ni2;
      int consumed = 0;
      sscanf(p, "%d/%d/%d %d/%d/%d %d/%d/%d%n", &vi0, &ti0, &ni0, &vi1, &ti1, &ni1, &vi2, &ti2, &ni2, &consumed);
      p += consumed;

      model->faces[fi][0] = vi0;
      model->faces[fi][1] = ti0;
      model->faces[fi][2] = ni0;

      model->faces[fi][3] = vi1;
      model->faces[fi][4] = ti1;
      model->faces[fi][5] = ni1;      

      model->faces[fi][6] = vi2;
      model->faces[fi][7] = ti2;
      model->faces[fi][8] = ni2;

      fi++;
    }

    p++;
  }

  model->vcount = vi;
  model->fcount = fi;
  printf("read %d vertices, %d faces\n", model->vcount, model->fcount);

  float dx = maxx - minx;
  float dy = maxy - miny;
  float dz = maxz - minz;
  float scale = 1.0 / dx;
  if (dy > dx) {
    scale = 1.0 / dy;
  }
  if ((dz > dy) && (dz > dx)) {
    scale = 1.0 / dz;
  }

  for (int i = 0; i < model->vcount; i++) {
    model->vertices[i][0] = model->vertices[i][0] * scale * 400 + 400;
    model->vertices[i][1] = model->vertices[i][1] * scale * 300 + 300;
    model->vertices[i][2] = model->vertices[i][2] * scale;
  }  
}

static void initialize(GlobalState *state)
{
  initialized = true;
  load_model(state, &model);
}

#define WHITE 0xFFFFFFFF
#define BLACK 0xFF000000

static void clear_buffer(DrawingBuffer *buffer, uint32_t color)
{
  for (int j = 0; j <= buffer->height; j++) {
    for (int i = 0; i <= buffer->width; i++) {
      set_pixel(buffer, i, j, color);
    }
  }  
}

C_LINKAGE void draw_frame(GlobalState *state, DrawingBuffer *drawing_buffer)
{
  if (!initialized) {
    initialize(state);
  }

  clear_buffer(drawing_buffer, BLACK);

  // Vertex v0 = {50, 50, 0, 1, 0, 0};
  // Vertex v1 = {750, 50, 0, 0, 1, 0};
  // Vertex v2 = {400, 550, 0, 0, 0, 1};
  // draw_triangle(drawing_buffer, &v0, &v1, &v2);

  for (int fi = 0; fi < model.fcount; fi++) {
    int vi0 = model.faces[fi][0] - 1;
    int vi1 = model.faces[fi][3] - 1;
    int vi2 = model.faces[fi][6] - 1;

    float *sv0 = model.vertices[vi0];
    float *sv1 = model.vertices[vi1];
    float *sv2 = model.vertices[vi2];

    Vertex v0 = {sv0[0], sv0[1], sv0[2], 1, 0, 0};
    Vertex v1 = {sv1[0], sv1[1], sv1[2], 0, 1, 0};
    Vertex v2 = {sv2[0], sv2[1], sv2[2], 0, 0, 1};
    draw_triangle(drawing_buffer, &v0, &v1, &v2);

    // draw_line(drawing_buffer, model.vertices[vi0][0], model.vertices[vi0][1], model.vertices[vi1][0], model.vertices[vi1][1], WHITE);
    // draw_line(drawing_buffer, model.vertices[vi1][0], model.vertices[vi1][1], model.vertices[vi2][0], model.vertices[vi2][1], WHITE);
    // draw_line(drawing_buffer, model.vertices[vi2][0], model.vertices[vi2][1], model.vertices[vi0][0], model.vertices[vi0][1], WHITE);
  }
}
