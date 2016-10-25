#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "game.h"

#define ASSERT(x) if (!(x)) { printf("Assertion at %s, line %d failed: %s\n", __FILE__, __LINE__, #x); *((uint32_t *)1) = 0xDEADCAFE; }

#include "math.cpp"
#include "tga.cpp"
#include "model.cpp"

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

static inline void set_pixel(DrawingBuffer *buffer, int32_t x, int32_t y, Vec3f color)
{
  uint8_t r = (uint8_t) (255.0 * color.r);
  uint8_t g = (uint8_t) (255.0 * color.g);
  uint8_t b = (uint8_t) (255.0 * color.b);
  uint32_t c = 0xFF000000 | (r << 16) | (g << 8) | b;

  int idx = (y * buffer->pitch + x) * buffer->bits_per_pixel;
  *((uint32_t *) &((uint8_t *)buffer->pixels)[idx]) = c;
}

#define ZBUFFER_MAX 0xFFFFFFFF

typedef struct RenderingContext {
  DrawingBuffer *target;
  uint32_t *zbuffer;

  TgaImage *diffuse;
  Model *model;

  Vec3f clear_color;
  Vec3f light;

  Mat44 modelview;
  Mat44 projection;
  Mat44 viewport;
  Mat44 mvp;
} RenderingContext;

typedef struct Shader {
  Vec3f positions[3];
  Vec3f normals[3];
  Vec3f texture_coords[3];
  Vec3f colors[3];

  void vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color);
  bool fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color);
} Shader;

inline static float edge_func(float x0, float y0, float x1, float y1)
{
  return (x0 * y1) - (x1 * y0);
}

void Shader::vertex(RenderingContext *ctx, int idx, Vec3f position, Vec3f normal, Vec3f texture, Vec3f color)
{
  positions[idx] = position * ctx->mvp;
  normals[idx] = (normal * ctx->modelview).normalized(); // TODO: Calculate correct normal
  texture_coords[idx] = texture;
  colors[idx] = color;
}


bool Shader::fragment(RenderingContext *ctx, float t0, float t1, float t2, Vec3f *color)
{
  Vec3f normal = normals[0] * t0 + normals[1] * t1 + normals[2] * t2;
  float intensity = normal.dot(ctx->light);

  Vec3f vcolor = (Vec3f){colors[0].r * t0 + colors[1].r * t1 + colors[2].r * t2,
                         colors[0].g * t0 + colors[1].g * t1 + colors[2].g * t2,
                         colors[0].b * t0 + colors[1].b * t1 + colors[2].b * t2};

  Vec3f tex = texture_coords[0] * t0 + texture_coords[1] * t1 + texture_coords[2] * t2;
  int texX = tex.x * ctx->diffuse->width;
  int texY = tex.y * ctx->diffuse->height;

  Vec3f tcolor = ctx->diffuse->pixels[texY*ctx->diffuse->width+texX];
  *color = (Vec3f){vcolor.r * tcolor.r, vcolor.g * tcolor.g, vcolor.b * tcolor.b} * intensity;

  if (color->r < 0) { color->r = 0.0; }
  if (color->g < 0) { color->g = 0.0; }
  if (color->b < 0) { color->b = 0.0; }
        
  return true;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))

static void draw_triangle(RenderingContext *ctx, Shader *shader)
{
  Vec3f p0 = shader->positions[0] * ctx->viewport;
  Vec3f p1 = shader->positions[1] * ctx->viewport;
  Vec3f p2 = shader->positions[2] * ctx->viewport;

  int minx = MIN3(p0.x, p1.x, p2.x);
  int miny = MIN3(p0.y, p1.y, p2.y);
  int maxx = MAX3(p0.x, p1.x, p2.x);
  int maxy = MAX3(p0.y, p1.y, p2.y);

  int target_width = ctx->target->width;
  int target_height = ctx->target->height;

  // Clip bounding rect to target rect
  minx = MAX(0, minx);
  miny = MAX(0, miny);
  maxx = MIN(maxx, target_width - 1);
  maxy = MIN(maxy, target_height - 1);

  for (int j = miny; j <= maxy; j++) {
    bool inside = false;

    for (int i = minx; i <= maxx; i++) {
      float tx = i + 0.5;
      float ty = j + 0.5;

      float area = edge_func(p1.x - p0.x, p1.y - p0.y, p2.x - p1.x, p2.y - p1.y);
      float t0 = edge_func(p2.x - p1.x, p2.y - p1.y, tx - p1.x, ty - p1.y);
      float t1 = edge_func(p0.x - p2.x, p0.y - p2.y, tx - p2.x, ty - p2.y);
      float t2 = edge_func(p1.x - p0.x, p1.y - p0.y, tx - p0.x, ty - p0.y);

      if (t0 >= 0.0 && t1 >= 0.0 && t2 >= 0.0) {
        inside = true;

        t0 /= area;
        t1 /= area;
        t2 /= area;

        uint32_t zvalue = (1 - (p0.z * t0 + p1.z * t1 + p2.z * t2)) * ZBUFFER_MAX;
        if (zvalue > ctx->zbuffer[j*target_width+i]) {
          ctx->zbuffer[j*target_width+i] = zvalue;

          Vec3f color;
          if (shader->fragment(ctx, t0, t1, t2, &color)) {
            set_pixel(ctx->target, i, j, color);
          }
        }
      } else {
        if (inside) {
          break;
        }
      }
    }
  }
}

static void load_model(GlobalState *state, char *filename, Model *model)
{
  FileContents contents = state->platform_api.read_file_contents(filename);
  if (contents.size <= 0) {
    printf("Failed to load model\n");
    return;
  }

  printf("Read model file of %d bytes\n", contents.size);
  model_read(contents.bytes, contents.size, model);
}

static void load_texture(GlobalState *state, char *filename, TgaImage *target)
{
  FileContents contents = state->platform_api.read_file_contents(filename);
  if (contents.size <= 0) {
    printf("Failed to load texture\n");
    return;
  }

  printf("Read texture file of %d bytes\n", contents.size);
  tga_read_image(contents.bytes, contents.size, target);
}

static void clear_buffer(RenderingContext *ctx)
{
  for (int j = 0; j <= ctx->target->height; j++) {
    for (int i = 0; i <= ctx->target->width; i++) {
      set_pixel(ctx->target, i, j, ctx->clear_color);
    }
  }  
}

static void clear_zbuffer(RenderingContext *ctx)
{
  for (int i = 0; i < 800; i++) {
    for (int j = 0; j < 600; j++) {
      ctx->zbuffer[j*800+i] = 0;
    }
  }
}

static Mat44 projection_matrix(float near, float far, float fov)
{
  Mat44 result = Mat44::identity();
  result.k = (-far / (far - near));
  result.o = (-(far * near) / (far - near));
  result.l = -1.0;
  result.p = 0.0;

  float s = 1.0 / (tan(fov / 2.0 * PI / 180.0));
  result.a = s;
  result.f = s;

  return result;
}

static Mat44 viewport_matrix(float width, float height)
{
  Mat44 result = Mat44::identity();

  float hw = width / 2.0;
  float hh = height / 2.0;

  result.a = hw / (hw / hh);
  result.m = hw;
  result.f = hh;
  result.n = hh;

  return result;
}

static bool initialized = false;

static TgaImage diffuseTexture;
static Model model;
static uint32_t zbuffer[800][600];
static RenderingContext rendering_context;
static Shader shader;

static float angle = 0;
static float hue = 0.0;

#define WHITE (Vec3f){1, 1, 1}
#define BLACK (Vec3f){0, 0, 0}

static void initialize(GlobalState *state, DrawingBuffer *buffer)
{
  initialized = true;

  RenderingContext *ctx = &rendering_context;
  ctx->target = buffer;
  ctx->clear_color = BLACK;

  ctx->viewport = viewport_matrix(buffer->width, buffer->height);
  ctx->projection = projection_matrix(0.1, 10, 90);
  ctx->light = ((Vec3f){0, -1, 0}).normalized();

  // TODO: Allocate dynamically
  ctx->model = &model;
  ctx->diffuse = &diffuseTexture;
  ctx->zbuffer = (uint32_t *) &zbuffer;

  load_model(state, (char *) "data/rabbit/rabbit.obj", rendering_context.model);
  load_texture(state, (char *) "data/rabbit/diffuse.tga", rendering_context.diffuse);
}

static void render_model(RenderingContext *ctx, Model *model, bool wireframe = false)
{
  angle += 0.005;
  if (angle > 2*PI) {
    angle -= 2*PI;
  }

  // hue += 0.5;
  // if (hue > 360.0) {
  //   hue -= 360.0;
  // }

  // float r, g, b;
  // hsv_to_rgb(hue, 1.0, 1.0, &r, &g, &b);
  Vec3f color = {1, 1, 1}; //{r, g, b};

  Mat44 cam_mat = Mat44::translate(0, 0, 1) * Mat44::rotate_y(angle);
  Mat44 model_mat = Mat44::translate(0, -0.4, 0);

  ctx->modelview = model_mat * cam_mat.inverse();
  ctx->mvp = ctx->modelview * ctx->projection;

  for (int fi = 0; fi < model->fcount; fi++) {
    int *face = model->faces[fi];

    for (int vi = 0; vi < 3; vi++) {
      Vec3f position = model->vertices[face[vi*3]];
      Vec3f texture = model->texture_coords[face[vi*3+1]];
      Vec3f normal = model->normals[face[vi*3+2]];

      shader.vertex(ctx, vi, position, normal, texture, color);
    }

    draw_triangle(ctx, &shader);
  }  
}

C_LINKAGE void draw_frame(GlobalState *state, DrawingBuffer *drawing_buffer)
{
  if (!initialized) {
    initialize(state, drawing_buffer);
  }

  clear_buffer(&rendering_context);
  clear_zbuffer(&rendering_context);

  render_model(&rendering_context, &model);
}
