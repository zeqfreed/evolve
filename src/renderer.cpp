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

typedef union Vec3f {
  struct {
    float x;
    float y;
    float z;
  };

  struct {
    float r;
    float g;
    float b;
  };

  Vec3f cross(Vec3f v);
  float dot(Vec3f v);
  float length();
  Vec3f normalized();
} Vec3f;

typedef struct Vertex {
  Vec3f pos;
  Vec3f color;
  Vec3f normal;
} Vertex;


inline Vec3f operator+(Vec3f a, Vec3f b)
{
  return (Vec3f){a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3f operator-(Vec3f a, Vec3f b)
{
  return (Vec3f){a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3f operator-(Vec3f a)
{
  return (Vec3f){-a.x, -a.y, -a.z};
}

inline Vec3f operator*(Vec3f v, float scalar)
{
  return (Vec3f){v.x * scalar, v.y * scalar, v.z * scalar};
}

inline Vec3f Vec3f::cross(Vec3f v)
{
  float nx = y * v.z - z * v.y;
  float ny = z * v.x - x * v.z;
  float nz = x * v.y - y * v.x;
  return (Vec3f){nx, ny, nz};
}

inline float Vec3f::dot(Vec3f v)
{
  return x * v.x + y * v.y + z * v.z;
}

inline float Vec3f::length()
{
  return sqrt(x * x + y * y + z * z);
}

inline Vec3f Vec3f::normalized()
{
  float factor = 1.0 / this->length();
  return (Vec3f){x *= factor, y *= factor, z *= factor};
}

inline static float edge_func(float x0, float y0, float x1, float y1)
{
  return (x0 * y1) - (x1 * y0);
}

static float zbuffer[800][600];

static void clear_zbuffer()
{
  for (int i = 0; i < 800; i++) {
    for (int j = 0; j < 600; j++) {
      zbuffer[i][j] = -1000000.0;
    }
  }
}

static void draw_triangle(DrawingBuffer *buffer, Vertex *v0, Vertex *v1, Vertex *v2)
{
  Vec3f p0 = v0->pos;
  Vec3f p1 = v1->pos;
  Vec3f p2 = v2->pos;

  int minx = p0.x;
  if (p1.x < minx) { minx = p1.x; }
  if (p2.x < minx) { minx = p2.x; }

  int miny = p0.y;
  if (p1.y < miny) { miny = p1.y; }
  if (p2.y < miny) { miny = p2.y; }  

  int maxx = p0.x;
  if (p1.x > maxx) { maxx = p1.x; }
  if (p2.x > maxx) { maxx = p2.x; }  

  int maxy = p0.y;
  if (p1.y > maxy) { maxy = p1.y; }
  if (p2.y > maxy) { maxy = p2.y; }

  // Flat shading
  // Vec3f normal = ((p2 - p0).cross(p1 - p0)).normalized();
  // float intensity = normal.dot((Vec3f){0, 0, -1});

  for (int j = miny; j <= maxy; j++) {
    for (int i = minx; i <= maxx; i++) {
      float tx = i + 0.5;
      float ty = j + 0.5;

      float area = edge_func(p1.x - p0.x, p1.y - p0.y, p2.x - p1.x, p2.y - p1.y);
      float t0 = edge_func(p2.x - p1.x, p2.y - p1.y, tx - p1.x, ty - p1.y);
      float t1 = edge_func(p0.x - p2.x, p0.y - p2.y, tx - p2.x, ty - p2.y);
      float t2 = edge_func(p1.x - p0.x, p1.y - p0.y, tx - p0.x, ty - p0.y);

      if ((t0 >= 0.0 && t1 >= 0.0 && t2 >= 0.0) ||
         (t0 <= 0.0 && t1 <= 0.0 && t2 <= 0.0)) {
        t0 /= area;
        t1 /= area;
        t2 /= area;

        Vec3f normal = -(v0->normal * t0 + v1->normal * t1 + v2->normal * t2);
        float intensity = normal.dot((Vec3f){0, 0, -1});

        Vec3f color = (Vec3f){v0->color.r * t0 + v1->color.r * t1 + v2->color.r * t2,
                       v0->color.g * t0 + v1->color.g * t1 + v2->color.g * t2,
                       v0->color.b * t0 + v1->color.b * t1 + v2->color.b * t2} * intensity;
        
        if (color.r < 0) { color.r = 0.0; }
        if (color.g < 0) { color.g = 0.0; }
        if (color.b < 0) { color.b = 0.0; }
        uint8_t r = (uint8_t) (255.0 * color.r);
        uint8_t g = (uint8_t) (255.0 * color.g);
        uint8_t b = (uint8_t) (255.0 * color.b);

        float zvalue = p0.z * t0 + p1.z * t1 + p2.z * t2;
        if (zvalue > zbuffer[i][j]) {
          zbuffer[i][j] = zvalue;
          set_pixel(buffer, i, j, 0xFF000000 | (r << 16) | (g << 8) | b);
        }
      }
    }
  }
}

static bool initialized = false;

typedef struct Model {
  int vcount;
  int tcount;
  int ncount;
  int fcount;
  Vec3f vertices[2048];
  Vec3f normals[2048];
  float texture_coords[2048][3];
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
  int ni = 0;

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
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) != 3) {
          continue;
        }
        p += consumed;

        model->vertices[vi++] = (Vec3f){x, y, z};

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
      } else if (*p == 'n') {
        p++;
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) != 3) {
          continue;
        }
        p += consumed;

        model->normals[ni++] = (Vec3f){x, y, z};
      }
    } else if (*p == 'f') {
      p++;

      int vi0, ti0, ni0, vi1, ti1, ni1, vi2, ti2, ni2;
      int consumed = 0;
      if (sscanf(p, "%d/%d/%d %d/%d/%d %d/%d/%d%n", &vi0, &ti0, &ni0, &vi1, &ti1, &ni1, &vi2, &ti2, &ni2, &consumed) != 9) {
        continue;
      }
      p += consumed;

      int *face = model->faces[fi++];
      face[0] = vi0 - 1;
      face[1] = ti0 - 1;
      face[2] = ni0 - 1;

      face[3] = vi1 - 1;
      face[4] = ti1 - 1;
      face[5] = ni1 - 1;

      face[6] = vi2 - 1;
      face[7] = ti2 - 1;
      face[8] = ni2 - 1;
    }

    p++;
  }

  model->vcount = vi;
  model->fcount = fi;
  model->ncount = ni;
  printf("read %d vertices, %d faces, %d normals\n", model->vcount, model->fcount, model->ncount);

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
    model->vertices[i].x = model->vertices[i].x * scale * 400;
    model->vertices[i].y = model->vertices[i].y * scale * 400;
    model->vertices[i].z = model->vertices[i].z * scale * 400;
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

typedef struct Mat33 {
  float m[3][3];
} Mat33;

static Mat33 rotation_mat(float angle)
{
  Mat33 result = {};

  result.m[0][0] = cos(angle);
  result.m[1][0] = sin(angle);
  result.m[0][1] = -sin(angle);
  result.m[1][1] = cos(angle);
  result.m[2][2] = 1.0;

  return result;
}

static Vec3f vec_mat_mul(Vec3f vec, Mat33 mat)
{
  Vec3f result = {};

  result.x = vec.x * mat.m[0][0] + vec.y * mat.m[0][1] + vec.z * mat.m[0][2];
  result.y = vec.x * mat.m[1][0] + vec.y * mat.m[1][1] + vec.z * mat.m[1][2];
  result.z = vec.x * mat.m[2][0] + vec.y * mat.m[2][1] + vec.z * mat.m[2][2];

  return result;
}


#define PI 3.1415926
static float angle = 0;

static void render_triangle(DrawingBuffer *buffer)
{
  Vec3f p0 = {-100, -100, 0};
  Vec3f p1 = {100, -100, 0};
  Vec3f p2 = {0, 100, 0};

  Mat33 mat = rotation_mat(angle);
  p0 = vec_mat_mul(p0, mat);
  p1 = vec_mat_mul(p1, mat);
  p2 = vec_mat_mul(p2, mat);

  p0.x += 400;
  p0.y += 300;
  p1.x += 400;
  p1.y += 300;
  p2.x += 400;
  p2.y += 300;  

  Vertex v0 = {p0, {1, 0, 0}};
  Vertex v1 = {p1, {0, 1, 0}};
  Vertex v2 = {p2, {0, 0, 1}};

  clear_zbuffer();
  draw_triangle(buffer, &v0, &v1, &v2);

  angle += 0.005;
  if (angle > 2*PI) {
    angle -= 2*PI;
  }  
}

static float hue = 0.0;

#define ASSERT(x) if (!(x)) { printf("Assertion at %s, line %d failed: %s\n", __FILE__, __LINE__, #x); *((uint32_t *)1) = 0xDEADCAFE; }

static void render_model(DrawingBuffer *buffer)
{
 Mat33 mat = rotation_mat(angle);
  //angle += 0.005;
  if (angle > 2*PI) {
    angle -= 2*PI;
  }

  hue += 0.5;
  if (hue > 360.0) {
    hue -= 360.0;
  }

  float r, g, b;
  hsv_to_rgb(hue, 1.0, 1.0, &r, &g, &b);
  Vec3f color = {r, g, b};

  clear_zbuffer();

  for (int fi = 0; fi < model.fcount; fi++) {
    int *face = model.faces[fi];

    Vertex v0 = {};
    Vertex v1 = {};
    Vertex v2 = {};

    v0.color = color;
    v0.normal = model.normals[face[2]];
    v1.color = color;
    v1.normal = model.normals[face[5]];
    v2.color = color;
    v2.normal = model.normals[face[8]];

    Vec3f p0 = model.vertices[face[0]];
    Vec3f p1 = model.vertices[face[3]];
    Vec3f p2 = model.vertices[face[6]];

    v0.pos = vec_mat_mul(p0, mat);
    v0.pos.x += 400;
    v0.pos.y += 300;

    v1.pos = vec_mat_mul(p1, mat);
    v1.pos.x += 400;
    v1.pos.y += 300;

    v2.pos = vec_mat_mul(p2, mat);
    v2.pos.x += 400;
    v2.pos.y += 300;

    draw_triangle(buffer, &v0, &v1, &v2);
  }  
}

C_LINKAGE void draw_frame(GlobalState *state, DrawingBuffer *drawing_buffer)
{
  if (!initialized) {
    initialize(state);
  }

  clear_buffer(drawing_buffer, BLACK);
  //render_triangle(drawing_buffer);
  render_model(drawing_buffer);
}
